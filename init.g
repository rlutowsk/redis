#
# redis: connect to redis database
#
# Reading the declaration part of the package.
#
if not LoadKernelExtension("redis") then
  Error("failed to load kernel module of package redis");
fi;
ReadPackage( "redis", "gap/redis.gd");
