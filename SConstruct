env = Environment()
env.AppendUnique(CFLAGS=["-std=c99", "-Wall"])

buildDir = Dir("build")

SConscript("src/SConscript", exports="env", variant_dir=buildDir, duplicate=0)
Clean(".", buildDir)