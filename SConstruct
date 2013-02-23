env = Environment()

buildType = ARGUMENTS.get("BuildType", "DEBUG").upper()
if buildType == "DEBUG":
  env.AppendUnique(CCFLAGS="-g")
elif buildType == "RELEASE":
  env.AppendUnique(CCFLAGS="-O3")
else:
  raise RuntimeError("Unknown build type '%s'" % buildType)

env.AppendUnique(CFLAGS=["-std=c99", "-Wall"])

buildDir = Dir("build")

SConscript("src/SConscript", exports="env", variant_dir=buildDir, duplicate=0)
Clean(".", buildDir)