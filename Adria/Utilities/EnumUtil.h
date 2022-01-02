#pragma once

namespace adria
{

#define DEFINE_ENUM_BIT_OPERATORS(ENUMTYPE, UNDERLYING_TYPE) \
inline ENUMTYPE operator | (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((UNDERLYING_TYPE)a) | ((UNDERLYING_TYPE)b)); } \
inline ENUMTYPE &operator |= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((UNDERLYING_TYPE &)a) |= ((UNDERLYING_TYPE)b)); } \
inline ENUMTYPE operator & (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((UNDERLYING_TYPE)a) & ((UNDERLYING_TYPE)b)); } \
inline ENUMTYPE &operator &= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((UNDERLYING_TYPE &)a) &= ((UNDERLYING_TYPE)b)); } \
inline ENUMTYPE operator ~ (ENUMTYPE a) { return ENUMTYPE(~((UNDERLYING_TYPE)a)); } \
inline ENUMTYPE operator ^ (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((UNDERLYING_TYPE)a) ^ ((UNDERLYING_TYPE)b)); } \
inline ENUMTYPE &operator ^= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((UNDERLYING_TYPE &)a) ^= ((UNDERLYING_TYPE)b)); } \

}