import os, random, string, sys, pwd, grp, stat, math
# import numpy
from pwd import getpwuid
from time import time

def entropy2(labels):
    """ Computes entropy of label distribution. """
    n_labels = len(labels)

    if n_labels <= 1:
        return 0
    tmp=[]
    for c in labels:
        tmp.append(ord(c))

    counts = []
    for i in range(257):
        counts.append(0)

    for i in range(len(tmp)):
        counts[tmp[i]] += 1 

    # counts = numpy.bincount(tmp)
    probs = []
    for i in counts:
        probs.append(1.0 * i / n_labels)

    n_classes = 0
    for i in probs:
        if i != 0:
            n_classes += 1
    # n_classes = numpy.count_nonzero(probs)

    if n_classes <= 1:
        return 0

    ent = 0.

    # Compute standard entropy.
    for i in probs:
        if i != 0:
            ent -= i * math.log(i, n_classes)

    return ent


def random_modify(bkdir,per_num,per_content,target_size):
    _bkdir = bkdir
    _per_num = float(per_num)
    _per_content = float(per_content)
    _target_size = int(target_size)

    fileList = []

    """ Figure out # of files to modify """
    for root, subdir, files in os.walk(_bkdir):
        for f in files:
            f = os.path.join(root,f)
            fileList.append(f)
    print "Total number of files: %d"%len(fileList)
    target_num = int(len(fileList)*_per_num)
    #target_num = 20


    cnt = 0
    msize = 0
    max_msize = 0
    min_msize = 999999
    """ randomly modify 2% of the files """
    """ excluding files from /sys, /proc, /dev/pts, lib and the 4 scripts used"""
    while cnt < target_num:
        sel_f = random.randrange(len(fileList))
        rdir = fileList[sel_f]
        fileList.remove(fileList[sel_f])

        #print "Selected file: "+rdir
        try:
            if os.path.islink(rdir):
        #       print "Invalid File:"+rdir
                raise Exception("Invalid File!")
        except:
            continue
            

        try:
            #if getpwuid(os.stat(rdir).st_uid).pw_name != 'root':
            if os.stat(rdir).st_size > 0:
                f = open(rdir,'r+')
                size = os.stat(rdir).st_size
                md_size = int(size*_per_content)
                pos = random.randrange(size-md_size+1024)
                f.seek(pos)
                orig_cont = f.read(min(md_size,size-pos))
                ent = entropy2(orig_cont)
                msize=(msize*cnt+md_size)/(cnt+1)
                if md_size > max_msize:
                        max_msize = md_size
                if md_size < min_msize:
                        min_msize = md_size
                while md_size > 2048:
                        f.write(bytearray(os.urandom(1024)))
                        for i in range(1,1025):
                                f.write("a")
                        md_size = md_size - 2048

                f.write(bytearray(os.urandom(int(md_size*ent))))
                for i in range(1,md_size-int(md_size*ent)+1):
                        f.write("a")

                cnt=cnt+1
                #print "%d-th file: %s successfully modified."%(cnt,rdir)
            #else:
                #continue
        except OSError:
            print ("One OSError")
            continue

    print "Modified %d files, avg modify size: %d, min: %d, max: %d"%(cnt,msize,min_msize,max_msize)
    """ randomly generate 200MB files """
    cur_size = 0
    while _target_size - cur_size > 1024:
        rdir = _bkdir
        while(os.path.isdir(rdir)):
            if os.listdir(rdir):
                tmpdir = random.choice(os.listdir(rdir))
                rdir = rdir+tmpdir+"/"
            else:
                break

        tmp = rdir.split('/')
        dir1 = tmp[1]
        #print "DIR: "+dir1
        if dir1 in ['bin','lib64','media','opt','root','srv','tmp', 'usr', 'etc', 'home','lib','var']:
            if not os.path.isdir(rdir):
                rdir=rdir.split('/')
                rdir=string.join(rdir[:-2],'/')
                rdir=rdir+'/'+"%f"%time()
            else:
                rdir=rdir+"%f"%time()
            # print "generate success."
            f = open(rdir,'a+')
            size = random.randrange(_target_size-cur_size)
            f.write(bytearray(os.urandom(size*1024)))
            
            f.close()
            uid = pwd.getpwnam("nobody").pw_uid
            gid = grp.getgrnam("nogroup").gr_gid
            os.chown(rdir,uid,gid)
            cur_size = cur_size+size
            cnt=cnt+1
            msize=(msize*(cnt-1)+size*1024)/cnt
            #print "Create file: %s with %d KB"%(rdir,size)
    print "Average Write Size: %d Bytes, total modification: %d"%(msize,msize*cnt)
    

if __name__ == '__main__':
    if len(sys.argv) < 5:
        print 'Invalid parameters.'
        print 'Sample: python rrandomio.py "/" 0.02 0.1 200000'
        sys.exit()

    bkdir = sys.argv[1]
    per_num = sys.argv[2]
    per_content = sys.argv[3]
    target_size = sys.argv[4]
    print "Backup %s percent of %s with %s percent of content of each file modified.\n In addition, %s KB file are created."%(per_num,bkdir,per_content,target_size)
    random_modify(bkdir,per_num,per_content,target_size)
