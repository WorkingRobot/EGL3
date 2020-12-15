#pragma once

#define EXPAND( x ) x

// Make a FOREACH macro
#define FE_0(WHAT)
#define FE_1(WHAT, X) EXPAND(WHAT(X))
#define FE_2(WHAT, X, ...) EXPAND(WHAT(X)FE_1(WHAT, __VA_ARGS__))
#define FE_3(WHAT, X, ...) EXPAND(WHAT(X)FE_2(WHAT, __VA_ARGS__))
#define FE_4(WHAT, X, ...) EXPAND(WHAT(X)FE_3(WHAT, __VA_ARGS__))
#define FE_5(WHAT, X, ...) EXPAND(WHAT(X)FE_4(WHAT, __VA_ARGS__))
//... repeat as needed

#define GET_MACRO(_0,_1,_2,_3,_4,_5,NAME,...) NAME 
#define FOR_EACH(action,...) EXPAND(GET_MACRO(_0,__VA_ARGS__,FE_5,FE_4,FE_3,FE_2,FE_1,FE_0)(action,__VA_ARGS__))
