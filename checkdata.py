#!/usr/bin/env python3
# -*- coding:utf-8 -*-
# @Time    : 12/26/19 4:00 PM
# @Author  : kay
# @E-mail  : 861186267@qq.com

import os
import sys
import shutil



if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage python3.5 checkdata.py srcdir dstdir")
        exit(1)
    srcdir = sys.argv[1]
    dstdir = sys.argv[2]

    # srcdir = "/data2/alldata/kwsdata/zss"
    # dstdir = "/data1/ori_audio_data/alldata/checkdata/record"

    fkwsList = open("./kwsList.txt", "r")
    lkwsList = fkwsList.readlines()

    for line in lkwsList:
        kws = line.strip("\n")
        fkw = open("kw", "w")
        fkw.write(kws)
        fkw.close()
        tempCMD = "./template ./lexicon.dic ./kw ./kw.bin"
        os.system(tempCMD)

        wavdir = os.path.join(srcdir, kws, "oriwavdata")
        savedir = os.path.join(dstdir, kws)
        logdir = os.path.join(savedir, "log")

        if not os.path.isdir(savedir):
            os.makedirs(savedir)

        if not os.path.isdir(logdir):
            os.makedirs(logdir)

        logpath = os.path.join(logdir, "log.txt")
        checkCMD = "./test ./model.bin ./kw.bin ./kw " + wavdir + " " + logpath
        os.system(checkCMD)

        cpNowakeup = open("./nowakeLog", "r")
        for line in cpNowakeup:
            wavpath = line.strip("\n")
            if ".wav" not in wavpath:
                continue
            print("wavpath:{}".format(wavpath))
            labpath = wavpath[:-4] + ".lab"
            shutil.copy(wavpath, savedir)
            shutil.copy(labpath, savedir)

        cpNowakeup.close()
        nowakelogpath = os.path.join(logdir, "nowakeLog")
        shutil.move("./nowakeLog", nowakelogpath)
        os.remove("./kw")
        os.remove("./kw.bin")
