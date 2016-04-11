import json
import sys

def intersection(a,b):
    x = max(a[0], b[0])
    y = max(a[1], b[1])
    w = min(a[0]+a[2], b[0]+b[2]) - x
    h = min(a[1]+a[3], b[1]+b[3]) - y
    if w<0 or h<0:
        return 0.0
    return w*h*1.0

def frameIntersection(A,B):
    inter = 0.0
    for a in A:
        for b in B:
            inter += intersection(a,b)
    return inter

def FrameArea(framedat):
    area = 0.0
    for box in framedat:
        area += box[2]*box[3]*1.0
    return area

fp = open(sys.argv[2],'r')
out = json.loads(fp.read())

fp2 = open(sys.argv[1],'r')
boxes = fp2.read().split('\n')

frames = []
for data in out:
    frames.append(int(data['filename'].split('_')[-1].split('.')[0]))

reqBoxes = {}
for box in boxes:
    spl = box.split(',')
    if len(spl) == 5: 
        spl = [int(x) for x in spl]
        if spl[0] in frames:
            if spl[0] in reqBoxes:
                reqBoxes[spl[0]].append(spl[1:])
            else:
                reqBoxes[spl[0]] = [spl[1:]]

corrBoxes = {}

for data in out:
    rectdict = data['annotations']
    frame = int(data['filename'].split('_')[-1].split('.')[0])
    framedat = []
    for rect in rectdict:
        dat = [rect['x'], rect['y'], rect['width'], rect['height']]
        framedat.append(dat)
    corrBoxes[frame] = framedat

num = len(corrBoxes)
accuracy = 0
numer = 0
denom = 0

for frame in corrBoxes:
    if frame in reqBoxes:
        numer += frameIntersection(corrBoxes[frame], reqBoxes[frame]) 
        denom += FrameArea(corrBoxes[frame])
        val = frameIntersection(corrBoxes[frame], reqBoxes[frame])/FrameArea(corrBoxes[frame])
        print "Frame %d: %lf"%(frame, val)
        accuracy += val
    else:
        print "Frame %d: 0"
print "#"*50
print "Accuracy (validation set area) = %lf"%(accuracy/num)
print "Accuracy (validation set area) = %lf"%(numer/denom)
accuracy = 0
numer = 0
denom = 0
print "#"*50
for frame in corrBoxes:
    if frame in reqBoxes:
        numer += frameIntersection(corrBoxes[frame], reqBoxes[frame]) 
        denom += FrameArea(reqBoxes[frame])
        val = frameIntersection(corrBoxes[frame], reqBoxes[frame])/FrameArea(reqBoxes[frame])
        print "Frame %d: %lf"%(frame, val)
        accuracy += val
    else:
        print "Frame %d: 0"%(frame)
print "#"*50
print "Accuracy (output set area) = %lf"%(accuracy/num)
print "Accuracy (output set area) = %lf"%(numer/denom)
print "#"*50
