// R Jesse Chaney
// rchaney@pdx.edu

#ifndef _THREAD_HASH_H
# define _THREAD_HASH_H

# define OPTIONS "i:o:d:hvt:n"

# define MICROSECONDS_PER_SECOND 1000000.0

# ifndef NICE_VALUE
#  define NICE_VALUE 10
# endif // NICE_VALUE

# define FOREACH_ALGORITHM(ALGORITHM) \
        ALGORITHM(DES)           \
        ALGORITHM(NT)            \
        ALGORITHM(MD5)           \
        ALGORITHM(SHA256)        \
        ALGORITHM(SHA512)        \
        ALGORITHM(YESCRYPT)      \
        ALGORITHM(GOST_YESCRYPT) \
        ALGORITHM(BCRYPT)        \
        ALGORITHM(ALGORITHM_MAX)

# define GENERATE_ENUM(ENUM) ENUM,
# define GENERATE_STRING(STRING) #STRING,

typedef enum hash_algorithm_e {
    FOREACH_ALGORITHM(GENERATE_ENUM)
} hash_algorithm_t;

static const char *algorithm_string[] = {
    FOREACH_ALGORITHM(GENERATE_STRING)
};

#endif // _THREAD_HASH_H
