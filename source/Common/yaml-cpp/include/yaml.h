#ifndef YAML_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define YAML_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) ||                                            \
    (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || \
     (__GNUC__ >= 4))  // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include "parser.h"
#include "emitter.h"
#include "emitterstyle.h"
#include "stlemitter.h"
#include "exceptions.h"

#include "node/node.h"
#include "node/impl.h"
#include "node/convert.h"
#include "node/iterator.h"
#include "node/detail/impl.h"
#include "node/parse.h"
#include "node/emit.h"

#endif  // YAML_H_62B23520_7C8E_11DE_8A39_0800200C9A66
