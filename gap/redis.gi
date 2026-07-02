#
# redis: connect to redis database
#
# Implementations
#
InstallGlobalFunction( RedisCommand,
function(arg)
	return RedisCMD(CallFuncList(StringFormatted,arg));
end );

InstallGlobalFunction( RedisStatus,
function()
	return REDIS_STATUS;
end );

InstallGlobalFunction( RedisStatusMessage,
function()
	return REDIS_STATUS.message;
end );