#include<algorithm>
#include<cmath>
#include<assert.h>
#include<stdlib.h>

using namespace std;

// Global constants
float alpha = 0.5, beta = 1.3, ep1 = 4.0;

struct CodeWord
{
    float R, B, G, I_min, I_max, I_lo, I_hi, I_self;
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
	I_lo    = alpha*I_max;
	I_hi    = min(beta * I_max, I_min/alpha);
	I_self  = I;
    };

    void Update(int r,int g,int b,int time)
    {
	float I = sqrt(r*r + g*g + b*b);
	R = (R * freq + r)/(freq + 1);
	G = (G * freq + g)/(freq + 1);
	B = (B * freq + b)/(freq + 1);
	if(I < I_min) {
	    I_min = I;
	    I_hi = min(I_hi, I_min/alpha);
	}
	if(I > I_max) {
	    I_max = max(I_max, I);
	    I_lo = alpha*I_max;
	}
	freq++;
	mnrl = max(mnrl, (unsigned short)time - last);
	last = time;
	I_self = sqrt(R*R + G*G + B*B);
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

    bool Brightness(int r,int g,int b,float I)
    {
	return ((I >= I_lo) && (I <= I_hi));
    };
};

struct CodeBook
{
    CodeWord *head;

    void Init() { head = NULL; };

    void FindMatch(int r,int g,int b,int time)
    {
	CodeWord *curr,*prev;
	float I = sqrt(r*r + g*g + b*b);
	
	if(head == NULL) {
	    Update(NULL, NULL, r, g, b, time);
	}
	else {
	    if((head->ColorDist(r, g, b, I) <= ep1) && head->Brightness(r, g, b, I)) {
		Update(NULL, head, r, g, b, time);
		return;
	    }

	    if(head->next == NULL) {
		Update(NULL, NULL, r, g, b, time);
		return;
	    }

	    prev = head;
	    curr = head->next;
	    while(curr != NULL) {
		if((curr->ColorDist(r, g, b, I) <= ep1) && curr->Brightness(r, g, b, I)) {
		    Update(prev, curr, r, g, b, time);
		    return;
		}
		prev = curr;
		curr = curr->next;
	    }
	    Update(NULL, NULL, r, g, b, time);
	}
    };

    void Update(CodeWord *prev, CodeWord *curr, int r, int g, int b, int time)
    {
	if((prev == NULL) && (curr == NULL)) {
	    CodeWord *newWord = (CodeWord *)malloc(sizeof(CodeWord));
	    newWord->Init(r, g, b, time);
	    newWord->next = head;
	    head = newWord;
	}
	else if(prev == NULL) {
	    assert(head == curr);
	    curr->Update(r, g, b, time);
	}
	else {
	    CodeWord *curr = prev->next;
	    prev->next = curr->next;
	    curr->next = head;
	    head = curr;
	    head->Update(r, g, b, time);
	}
	return;
    };
};
