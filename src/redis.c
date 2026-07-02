/*
 * redis: connect to redis database
 */

#include <gap_all.h>    // GAP headers

#include <stdio.h>
#include <stdlib.h>
#include <hiredis/hiredis.h>

#ifdef DEBUG
#define INFO() fprintf( stderr, "%s:%d\n", __FUNCTION__, __LINE__ )
#else
#define INFO() {}
#endif

/* ***************************************************** */
/* ************** Functions' declarations ************** */

/* ************** Connection functions ***************** */
Obj FuncRedisConnected( Obj self );
Obj FuncRedisConnect( Obj self, Obj Hostname, Obj Port );
Obj FuncRedisFree( Obj self );
Obj FuncRedisPing( Obj self );

/* ************** Handle Redis commands ***************** */
Obj FuncCMD(Obj self, Obj Cmd);

/* ********************** Strings *********************** */
Obj FuncSetCounter(Obj self, Obj Name, Obj Value);
Obj FuncGetCounter(Obj self, Obj Name);
Obj FuncDEL(Obj self, Obj Name);

Obj FuncINCR( Obj self, Obj Name );
Obj FuncINCRBY( Obj self, Obj Name, Obj Value );

/* ********************** Lists ************************* */
Obj FuncPUSH(Obj self, Obj LR, Obj Name, Obj Data);
Obj FuncPOP(Obj self, Obj LR, Obj Name);

/* ******************* Sorted sets *********************** */
Obj FuncZRANGEBYSCORE( Obj self, Obj Set, Obj Start, Obj End );
Obj FuncRedisZADD( Obj self, Obj Set, Obj Score, Obj Elm );


/*
 * GVarFunc - list of functions to export
 */
static StructGVarFunc GVarFunc[] = {
    { "RedisConnect"         , 2, " address, port"        , FuncRedisConnect      , "redis.c:RedisConnect"   },
    { "RedisFree"            , 0, ""                      , FuncRedisFree         , "redis.c:RedisFree"      },
    { "RedisPing"            , 0, ""                      , FuncRedisPing         , "redis.c:RedisPing"      },
    { "RedisConnected"       , 0, ""                      , FuncRedisConnected    , "redis.c:RedisConnected" },
    { "RedisSetCounter"      , 2, " name, value"          , FuncSetCounter        , "redis.c:SetCounter"     },
    { "RedisGetCounter"      , 1, " name"                 , FuncGetCounter        , "redis.c:GetCounter"     },
    { "RedisIncrement"       , 1, " name"                 , FuncINCR              , "redis.c:Increment"      },
    { "RedisIncrementBy"     , 2, " name, value"          , FuncINCRBY            , "redis.c:IncrementBy"    },
    { "RedisLRPush"          , 3, " lr, name, data"       , FuncPUSH              , "redis.c:LRPush"         },
    { "RedisLRPop"           , 2, " lr, name"             , FuncPOP               , "redis.c:LRPop"          },
    { "RedisDelete"          , 1, " name"                 , FuncDEL               , "redis.c:Delete"         },
    { "RedisZRangeByScore"   , 3, " name, start, end"     , FuncZRANGEBYSCORE     , "redis.c:ZRangeByScore"  },
    { "RedisCMD"             , 1, " command_string"       , FuncCMD               , "redis.c:CMD"            },
    { 0 }
};

/*
 * Variables
 */

static redisContext *ctx;         // connection context

static redisReply *rep;           // always use rep = redisCommand(...)
static Obj        rval;           // always return rval GAP object

static Obj   status;              // make this a GAP record to store connection/command data:
static UInt  RNAM_ADDRESS,        // address of the Redis server
             RNAM_PORT,           // port
             RNAM_REPLY,          // integer holding response code
             RNAM_MESSAGE;        // string holding response/error message
static Int GVAR_REDIS_STATUS;     // hold the GAP variable - record as above

static Obj REDIS_REPLY_ERROR_OBJ; // handle the case when user tries to invoke
static Obj REDIS_NO_CTX_MSG;      // commands without connection

static Int InitLibrary(StructInitInfo * module)
{
    int gvar;

    InitGVarFuncsFromTable(GVarFunc);

    ctx = NULL;
    rep = NULL;

    /* define variables for no connection case */
    REDIS_NO_CTX_MSG      = MakeString("Redis context object not allocated, probably not connected.");
    REDIS_REPLY_ERROR_OBJ = INTOBJ_INT(REDIS_REPLY_ERROR);

    /* initialize the status record */
    RNAM_ADDRESS = RNamName("hostname");
    RNAM_PORT    = RNamName("port");
    RNAM_REPLY   = RNamName("reply");
    RNAM_MESSAGE = RNamName("message");

    status = NEW_PREC(0);
    AssPRec(status, RNAM_ADDRESS, MakeString(""));
    AssPRec(status, RNAM_PORT   , INTOBJ_INT(0));
    AssPRec(status, RNAM_REPLY  , INTOBJ_INT(0));
    AssPRec(status, RNAM_MESSAGE, MakeString(""));

    GVAR_REDIS_STATUS = GVarName("REDIS_STATUS");
    AssGVar(GVAR_REDIS_STATUS, status);
    MakeReadOnlyGVar( GVAR_REDIS_STATUS );

    /* define RESP2 and RESP3 answer values in GAP session */
    gvar = GVarName("REDIS_REPLY_STATUS");
    AssGVar(gvar, INTOBJ_INT(REDIS_REPLY_STATUS) );
    MakeReadOnlyGVar( gvar );

    gvar = GVarName("REDIS_REPLY_ERROR");
    AssGVar(gvar, INTOBJ_INT(REDIS_REPLY_ERROR) );
    MakeReadOnlyGVar( gvar );

    gvar = GVarName("REDIS_REPLY_INTEGER");
    AssGVar(gvar, INTOBJ_INT(REDIS_REPLY_INTEGER) );
    MakeReadOnlyGVar( gvar );

    gvar = GVarName("REDIS_REPLY_NIL");
    AssGVar(gvar, INTOBJ_INT(REDIS_REPLY_NIL) );
    MakeReadOnlyGVar( gvar );

    gvar = GVarName("REDIS_REPLY_STRING");
    AssGVar(gvar, INTOBJ_INT(REDIS_REPLY_STRING) );
    MakeReadOnlyGVar( gvar );

    gvar = GVarName("REDIS_REPLY_ARRAY");
    AssGVar(gvar, INTOBJ_INT(REDIS_REPLY_ARRAY) );
    MakeReadOnlyGVar( gvar );

    gvar = GVarName("REDIS_REPLY_DOUBLE");
    AssGVar(gvar, INTOBJ_INT(REDIS_REPLY_DOUBLE) );
    MakeReadOnlyGVar( gvar );

    gvar = GVarName("REDIS_REPLY_BOOL");
    AssGVar(gvar, INTOBJ_INT(REDIS_REPLY_BOOL) );
    MakeReadOnlyGVar( gvar );

    gvar = GVarName("REDIS_REPLY_MAP");
    AssGVar(gvar, INTOBJ_INT(REDIS_REPLY_MAP) );
    MakeReadOnlyGVar( gvar );

    gvar = GVarName("REDIS_REPLY_SET");
    AssGVar(gvar, INTOBJ_INT(REDIS_REPLY_SET) );
    MakeReadOnlyGVar( gvar );

    gvar = GVarName("REDIS_REPLY_PUSH");
    AssGVar(gvar, INTOBJ_INT(REDIS_REPLY_PUSH) );
    MakeReadOnlyGVar( gvar );

    gvar = GVarName("REDIS_REPLY_ATTR");
    AssGVar(gvar, INTOBJ_INT(REDIS_REPLY_ATTR) );
    MakeReadOnlyGVar( gvar );

    gvar = GVarName("REDIS_REPLY_BIGNUM");
    AssGVar(gvar, INTOBJ_INT(REDIS_REPLY_BIGNUM) );
    MakeReadOnlyGVar( gvar );

    gvar = GVarName("REDIS_REPLY_VERB");
    AssGVar(gvar, INTOBJ_INT(REDIS_REPLY_VERB) );
    MakeReadOnlyGVar( gvar );

    return 0;
}

static Int PostRestore(StructInitInfo * module)
{
    GVAR_REDIS_STATUS = GVarName("REDIS_STATUS");
    return 0;
}

static Int InitKernel (StructInitInfo * module)
{
    InitHdlrFuncsFromTable(GVarFunc);
    return 0;
}

static StructInitInfo module = {
    .type = MODULE_DYNAMIC,
    .name = "redis",
    .initKernel = InitKernel,
    .initLibrary = InitLibrary,
    .postRestore = PostRestore,
};

StructInitInfo * Init__Dynamic(void)
{
  return &module;
}


/* ***************************************************** */
/* ************** Functions' definitions *************** */
inline static void  SetStatusHostname(const Obj Hostname)
{
    AssPRec(status, RNAM_ADDRESS, Hostname);
}
inline static void SetStatusPort(const Obj Port)
{
    AssPRec(status, RNAM_PORT, Port);
}
inline static void SetStatusReply(const Obj Reply)
{
    AssPRec(status, RNAM_REPLY, Reply);
}
inline static void SetStatusReplyInt(const Int Reply)
{
    AssPRec(status, RNAM_REPLY, INTOBJ_INT(Reply) );
}
inline static void SetStatusMessage(const Obj Message)
{
    AssPRec(status, RNAM_MESSAGE, Message);
}
inline static void SetStatusMessageCstr(const char *Message)
{
    AssPRec(status, RNAM_MESSAGE, MakeString(Message) );
}

/* ***************************************************** */
/* ******** Handle responses with GAP objects ********** */

Obj ParseResponseGap(redisReply *rep);

static inline Obj PLAIN_LIST_REPLY(const redisReply *reply)
{
    Obj o = NEW_PLIST( T_PLIST, reply->elements );

    SET_LEN_PLIST( o, reply->elements );
    PLAIN_LIST( o );

    for (size_t i=0; i<reply->elements; i++) {
        ASS_LIST( o, i+1, ParseResponseGap( reply->element[i] ) );
        CHANGED_BAG(o);
    }
    return o;
}

static inline Obj PREC_REPLY(const redisReply *reply)
{
    Obj o = NEW_PREC(0);

    for (size_t i=0; i<reply->elements; i += 2) {
        // possibly do the same as for the lists, i.e. use ParseResponseGap for the value as well
        AssPRec(o, RNamName(reply->element[i]->str), MakeString(reply->element[i+1]->str));
    }
    return o;
}

Obj ParseResponseGap(redisReply *rep)
{
    switch(rep->type) {
        case REDIS_REPLY_STATUS:
            return MakeStringWithLen( rep-> str, rep->len );
        case REDIS_REPLY_ERROR:
            SetStatusMessage( MakeStringWithLen(rep->str, rep->len) );
            return Fail;
        case REDIS_REPLY_INTEGER:
            return INTOBJ_INT( rep->integer );
        case REDIS_REPLY_NIL:
            return MakeStringWithLen("",0);
        case REDIS_REPLY_VERB:
            /* this should probably do more ... */
        case REDIS_REPLY_STRING:
            return MakeStringWithLen( rep-> str, rep->len );
        case REDIS_REPLY_ARRAY:
        case REDIS_REPLY_SET:    /* these two will return a list */
            return PLAIN_LIST_REPLY( rep );
        case REDIS_REPLY_DOUBLE:
            return NEW_MACFLOAT( strtod( rep->str, NULL ) );
        case REDIS_REPLY_BOOL:
            return (rep->integer)?True:False;
        case REDIS_REPLY_PUSH:
            /* left for now */
            SetStatusMessageCstr( "PUSH response not implemented yet" );
            return Fail;
        case REDIS_REPLY_MAP:    /* return a record */
            return PREC_REPLY(rep);
        case REDIS_REPLY_BIGNUM:
            return IntStringInternal( NULL, rep->str );
    }
    return Fail;
}

/* check if ctx object is not null */
/* TODO: should we check for more? */
#define CheckCTX(obj) if (!ctx) { \
    SetStatusReply( REDIS_REPLY_ERROR_OBJ ); \
    SetStatusMessage( REDIS_NO_CTX_MSG ); \
    return obj; }

#define AssertArgType(test) if ( !(test) ) { \
    ErrorQuit("Bad arguments provided", 0L, 0L); \
    }

#define _begin(test) AssertArgType(test); CheckCTX(Fail); rval = (Obj)0; SetStatusMessageCstr("");

#define _end() freeReplyObject(rep); return rval;

#define _parse_response() SetStatusReplyInt(rep->type); \
    if ( rep->type == REDIS_REPLY_ERROR ) { \
        SetStatusMessageCstr( rep->str ); \
        freeReplyObject(rep); \
        return Fail; \
    }

Obj FuncRedisConnected( Obj self )
{
    CheckCTX(False);

    return True;
}

Obj FuncRedisConnect( Obj self, Obj Hostname, Obj Port )
{
    if ( ctx ) {
        SetStatusReply(REDIS_REPLY_ERROR_OBJ);
        SetStatusMessageCstr( "Redis context already allocated" );
        return Fail;
    }

    AssertArgType( IS_STRING( Hostname ) && IS_INTOBJ( Port ) );

    SetStatusHostname( Hostname );
    SetStatusPort( Port );

    ctx  = redisConnect( CSTR_STRING(Hostname), INT_INTOBJ(Port) );

    CheckCTX(Fail);

    if ( ctx->err ) {
        SetStatusReply( REDIS_REPLY_ERROR_OBJ );
        SetStatusMessage( MakeString(ctx->errstr) );
        return Fail;
    }
    return True;
}

Obj FuncRedisFree( Obj self )
{
    redisFree( ctx );
    ctx = NULL;

    return (Obj)0;
}

Obj FuncRedisPing( Obj self )
{
    _begin(1);

    rep = redisCommand( ctx, "PING" );

    _parse_response();

    rval = ( rep->type == REDIS_REPLY_STATUS ) ? MakeString( rep->str ) : Fail;

    _end();
}

Obj FuncCMD(Obj self, Obj Cmd)
{
    _begin( IS_STRING( Cmd ) );

    rep = redisCommand(ctx, CSTR_STRING(Cmd));

    SetStatusReplyInt(rep->type);

    rval = ParseResponseGap(rep);

    _end();
}

Obj FuncSetCounter(Obj self, Obj Name, Obj Value)
{
    _begin( IS_STRING( Name ) && IS_INTOBJ( Value ) );

    rep = redisCommand( ctx, "SET %s %d", CSTR_STRING( Name ), INT_INTOBJ( Value ));

    _parse_response();

    rval = ( rep->type == REDIS_REPLY_STRING || rep->type == REDIS_REPLY_STATUS ) ? Value : Fail;

    _end();
}

Obj FuncGetCounter(Obj self, Obj Name)
{
    _begin( IS_STRING( Name ) );

    rep = redisCommand( ctx, "GET %s", CSTR_STRING( Name ));

    _parse_response();

    rval = ( rep->type == REDIS_REPLY_STRING ) ? IntStringInternal( 0, rep->str ) : Fail ;

    _end();
}

Obj FuncINCR( Obj self, Obj Name )
{
    _begin( IS_STRING( Name ) );

    rep = redisCommand( ctx, "INCR %s", CSTR_STRING( Name ));

    _parse_response();

    rval = ( rep->type == REDIS_REPLY_INTEGER ) ? INTOBJ_INT(rep->integer) : Fail;

    _end();
}

Obj FuncINCRBY( Obj self, Obj Name, Obj Value )
{
    _begin( IS_STRING( Name ) && IS_INTOBJ(Value) );

    rep = redisCommand( ctx, "INCRBY %s %d", CSTR_STRING( Name ), INT_INTOBJ(Value));

    _parse_response();

    rval = ( rep->type == REDIS_REPLY_INTEGER ) ? INTOBJ_INT(rep->integer) : Fail;

    _end();
}

Obj FuncPUSH(Obj self, Obj LR, Obj Name, Obj Data)
{
    _begin( IS_INTOBJ(LR) && IS_STRING( Name ) && IS_STRING( Data ) );

    if (INT_INTOBJ(LR)==0) {
        /* push from the left */
        rep = redisCommand(ctx, "LPUSH %s %s", CSTR_STRING(Name), CSTR_STRING(Data));
    } else {
        rep = redisCommand(ctx, "RPUSH %s %s", CSTR_STRING(Name), CSTR_STRING(Data));
    }

    _parse_response();

    rval = ( rep->type == REDIS_REPLY_INTEGER ) ? INTOBJ_INT(rep->integer) : Fail;

    _end();
}

Obj FuncPOP(Obj self, Obj LR, Obj Name)
{
    _begin( IS_INTOBJ(LR) && IS_STRING( Name ) );

    if (INT_INTOBJ(LR)==0) {
        /* push from the left */
        rep = redisCommand(ctx, "LPOP %s", CSTR_STRING(Name));
    } else {
        rep = redisCommand(ctx, "RPOP %s", CSTR_STRING(Name));
    }
    _parse_response();

    rval = ( rep->type == REDIS_REPLY_STRING ) ? MakeString(rep->str) : Fail;

    _end();
}

Obj FuncDEL(Obj self, Obj Name)
{
    _begin( IS_STRING( Name ) );

    rep = redisCommand(ctx, "DEL %s", CSTR_STRING(Name));

    _parse_response();

    rval = (Obj)0;

    _end();
}

Obj FuncZRANGEBYSCORE( Obj self, Obj Set, Obj Start, Obj End )
{
    _begin( IS_STRING(Set) && IS_INTOBJ(Start) && IS_INTOBJ(End) );

    rep = redisCommand( ctx, "ZRANGEBYSCORE %s %d %d", CSTR_STRING(Set), INT_INTOBJ(Start), INT_INTOBJ(End) );

    _parse_response();

    rval = ( rep->type == REDIS_REPLY_ARRAY ) ? PLAIN_LIST_REPLY(rep) : Fail;

    _end();
}

Obj FuncRedisZADD( Obj self, Obj Set, Obj Score, Obj Elm )
{
    _begin( IS_STRING( Set ) && IS_INTOBJ( Score ) && IS_STRING( Elm ) );

    rep = redisCommand( ctx, "ZADD %s %d %s", CSTR_STRING(Set), INT_INTOBJ(Score), CSTR_STRING(Elm) );

    _parse_response();

    rval = ( rep->type == REDIS_REPLY_INTEGER ) ? INTOBJ_INT(rep->integer) : Fail;

    _end();
}

