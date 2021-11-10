#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
# scripts for comple shaders.
import sys
import os
import subprocess

if len(sys.argv) < 2:
	sys.exit("Please provide a target directory")

if not os.path.exists(sys.argv[1]):
	sys.exit("%s is not a valid directory" % sys.argv[1])

path = sys.argv[1]

# compile shaders to spv, shaders start with 'raytracing_' will compile online with shaderc.
exts = ('vert', 'frag', 'comp', 'geom', 'tesc', 'tese')
shaderfiles = [os.path.join(path, file) for file in os.listdir(path)
			   if file.split('.')[-1] in exts and 'raytracing_' not	in file]

failedshaders = []
for shaderfile in shaderfiles:
	# print("\n-------- %s --------\n" % shaderfile)
	if subprocess.call("glslc %s -o %s.spv" % (shaderfile, shaderfile), shell=True) != 0:
		failedshaders.append(shaderfile)

print("-------- Compilation result --------")

if len(failedshaders) == 0:
	print("SUCCESS: All shaders compiled to SPIR-V\n")
else:
	print("ERROR: %d shader(s) could not be compiled:\n" % len(failedshaders))
	for failedshader in failedshaders:
		print("\t" + failedshader)
