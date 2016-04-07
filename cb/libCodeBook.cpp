#include<algorithm>
#include<cmath>
#include<assert.h>
#include<stdlib.h>
#include<stdio.h>

using namespace std;

// Global con0.8nts
float alpha = 0.95, beta = 1.05, ep1 = 8.0, ep2 = 8.0, Gamma = 0.02, rho = 0.04;

// algo_phase = 0 for training and 1 for testing
extern int temporal_bound, algo_phase, num_frames;

enum pixel_class {
    FOREGROUND,
    BACKGROUND
};

struct CodeWord
{
    float R, B, G, I_min, I_max, I_lo, I_hi, I_self, sigma;
    int freq, mnrl, first, last;
    CodeWord *next;

    void Init(int r,int g,int b,int time)
    {
        float I = sqrt(r*r + g*g + b*b);
        R       = r;
        G       = g;
        B       = b;
        I_min   = I_max = I;
        freq    = 1;
        mnrl    = time - 1;
        first   = last = time;
        next    = NULL;
        sigma   = 10.0;         // Do we need to explicitly calculate this and initialise?
        I_lo    = alpha*I_max;
        I_hi    = min(beta * I_max, I_min/alpha);
        I_self  = I;
    };

    void Update(int r,int g,int b,int time)
    {
        float I = sqrt(r*r + g*g + b*b);
        I_self = sqrt(R*R + G*G + B*B);

        // R = (R * freq + r)/(freq + 1);
        // G = (G * freq + g)/(freq + 1);
        // B = (B * freq + b)/(freq + 1);

        // Adaptive updation
        R = Gamma * r + (1 - Gamma)*R;
        G = Gamma * g + (1 - Gamma)*G;
        B = Gamma * b + (1 - Gamma)*B;
        sigma = sqrt(pow(ColorDist(r, g, b, I_self),2)*rho + (1-rho)*pow(sigma,2));

        if(I < I_min) {
            I_min = I;
            I_hi = min(beta*I_max, I_min/alpha);
            I_lo = alpha*I_max;
        }
        if(I > I_max) {
            I_max = I;
            I_lo = alpha*I_max;
            I_hi = min(beta*I_max, I_min/alpha);
        }
        freq++;
        mnrl = max(mnrl, (unsigned short)time - last);
        last = time;
    };

    float ColorDist(int r,int g,int b,float I)
    {
        float p      = (R*r + G*g + B*b)/I_self;
        float q      = I*I - p*p;
        
        if(q <= 1e-2) {
            return 0.0;
        }
        else {
            return sqrt(q);
        }
    };

    float AdaptiveColorDist(int r,int g,int b,float I)
    {
        return ColorDist(r, g, b, I)/sigma;
    }

    bool Brightness(int r,int g,int b,float I)
    {
        return ((I >= I_lo) && (I <= I_hi));
    };

    bool DetectionBrightness(int r,int g,int b,float I)
    {
        return ((I >= 1.0*I_lo) && (I <= I_hi));
    };
};

struct CodeBook
{
    CodeWord *head[2];

    void Init() { head[0] = head[1] = NULL; };

    int Length(int layer)
    {
        int len = 0;
        CodeWord *curr = head[layer];
        while(curr != NULL) {
            len++;
            curr = curr->next;
        }
        return len;
    };

    void FindMatch(int layer,int r,int g,int b,int time)
    {
        CodeWord *curr,*prev;
        float I = sqrt(r*r + g*g + b*b);
        
        if(head[layer] == NULL) {
            Update(layer, NULL, NULL, r, g, b, time);
        }
        else {
            if((head[layer]->AdaptiveColorDist(r, g, b, I) <= ep1) && head[layer]->Brightness(r, g, b, I)) {
                Update(layer, NULL, head[layer], r, g, b, time);
                return;
            }

            if(head[layer]->next == NULL) {
                Update(layer, NULL, NULL, r, g, b, time);
                return;
            }

            prev = head[layer];
            curr = head[layer]->next;
            while(curr != NULL) {
                if((curr->AdaptiveColorDist(r, g, b, I) <= ep1) && curr->Brightness(r, g, b, I)) {
                    Update(layer, prev, curr, r, g, b, time);
                    return;
                }
                prev = curr;
                curr = curr->next;
            }
            Update(layer, NULL, NULL, r, g, b, time);
        }
    };

    void Update(int layer,CodeWord *prev, CodeWord *curr, int r, int g, int b, int time)
    {
        if((prev == NULL) && (curr == NULL)) {
            CodeWord *newWord = (CodeWord *)malloc(sizeof(CodeWord));
            newWord->Init(r, g, b, time);
            newWord->next = head[layer];
            head[layer] = newWord;
        }
        else if(prev == NULL) {
            assert(head[layer] == curr);
            curr->Update(r, g, b, time);
        }
        else {
            CodeWord *curr = prev->next;
            prev->next = curr->next;
            curr->next = head[layer];
            head[layer] = curr;
            head[layer]->Update(r, g, b, time);
        }
        return;
    };

    void WrapAround(int layer) {
        CodeWord *curr;
        curr = head[layer];
        while(curr != NULL) {
            curr->mnrl = max(curr->mnrl, num_frames - curr->last + curr->first - 1);
            curr = curr->next;
        }
    };

    void TemporalFit(int layer)
    {
        CodeWord *valid, *curr, *temp;

        assert(head[layer] != NULL);

        // Find the head[layer] of the new linked list which we want to construct
        curr = head[layer];
        while((curr != NULL) && (curr->mnrl > temporal_bound)) {
            head[layer] = head[layer]->next;
            free(curr);
            curr = head[layer];
        }
        if(curr == NULL)
            return;

        // Now construct the linked list by repeatedly checking the codewords
        // for the given condition
        valid = curr;
        curr = curr->next;
        while(curr != NULL) {
            if(curr->mnrl <= temporal_bound) {
                valid->next = curr;
                valid = valid->next;
                curr = curr->next;
            }
            else {
                valid->next = NULL;
                temp = curr->next;
                free(curr);
                curr = temp;
            }
        }
    };

    pixel_class DetectForeground(int layer,int r,int g,int b,int time)
    {
        CodeWord *curr,*prev;
        float I = sqrt(r*r + g*g + b*b);
        
        if(head[layer] == NULL) {
			if(layer == 1)
            	Update(layer, NULL, NULL, r, g, b, time);
            return FOREGROUND;
        }
        else {
            if((head[layer]->AdaptiveColorDist(r, g, b, I) <= ep2) && head[layer]->DetectionBrightness(r, g, b, I)) {
                Update(layer, NULL, head[layer], r, g, b, time);
                return BACKGROUND;
            }

            if(head[layer]->next == NULL) {
				if(layer == 1)
					Update(layer, NULL, NULL, r, g, b, time);
                return FOREGROUND;
            }

            prev = head[layer];
            curr = head[layer]->next;
            while(curr != NULL) {
                if((curr->AdaptiveColorDist(r, g, b, I) <= ep2) && curr->DetectionBrightness(r, g, b, I)) {
                    Update(layer, prev, curr, r, g, b, time);
                    return BACKGROUND;
                }
                prev = curr;
                curr = curr->next;
            }
			if(layer == 1)
            	Update(layer, NULL, NULL, r, g, b, time);
            return FOREGROUND;
        }
    };

};
