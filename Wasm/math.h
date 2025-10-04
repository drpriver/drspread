//
// Copyright © 2021-2025, David Priver <david@davidpriver.com>
//
#ifndef MATH_H
#define MATH_H
#include <allstd.h>
#ifdef __wasm__
__attribute__((import_name("log")))
extern double log(double);
__attribute__((import_name("pow")))
extern double pow(double, double);
__attribute__((import_name("round")))
extern double round(double);
#endif
#endif
