/* Version 2020.27.1
 * Copyright (c) 2019-2020 Guenther Brunthaler. All rights reserved.
 *
 * This source file is free software.
 * Distribution is permitted under the terms of the GPLv3. */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#define T(text) text

#define DIM(array) (sizeof(array) / sizeof *(array))

#define R4G_DEFINE_INIT_RPTR(resource_t, var_eq, dtor_member) \
   resource_t var_eq (void *)( \
      (char *)r4g.rlist - offsetof(resource_t, dtor_member) \
   )

typedef void (*r4g_action)(void);

struct {
   int errors;
   char const *static_error_message;
   r4g_action *rlist;
} r4g;

struct minimal_resource {
   r4g_action action, *saved;
};

static void release_c1(void) {
   while (r4g.rlist) (*r4g.rlist)();
}

static void release_after_c1(r4g_action *keep_this) {
   while (r4g.rlist != keep_this) (*r4g.rlist)();
}

static void error_c1(char const *static_message) {
   if (r4g.errors) {
      if (r4g.errors > 0 && ++r4g.errors >= 25) r4g.errors= -r4g.errors;
   } else {
      if (!r4g.static_error_message) r4g.static_error_message= static_message;
      r4g.errors= 1;
   }
   release_c1();
   assert(!r4g.rlist);
   exit(EXIT_FAILURE);
}

static void error_reporter_action(void) {
   R4G_DEFINE_INIT_RPTR(struct minimal_resource, *r=, action);
   r4g.rlist= r->saved;
   if (!r4g.errors) return;
   (void)fprintf(
      stderr, "%s\n", r4g.static_error_message
         ?  r4g.static_error_message
         :  T("Internal error!")
   );
   if (r4g.errors != 1) {
      (void)fprintf(
            stderr
         ,  r4g.errors < 0
            ? T("(%d or more follow-up errors also occurred.)\n")
            : T("(%d follow-up errors also occurred.)\n")
         ,  r4g.errors < 0 ? -r4g.errors : r4g.errors
      );
   }
}

static void raise_write_error_c1(void) {
   error_c1(T("Write error!"));
}

static void raise_read_error_c1(void) {
   error_c1(T("Read error!"));
}

static void fflush_c1(FILE *stream) {
   if (fflush(stream)) raise_write_error_c1();
}

static unsigned char sbox[1 << 8];
#define SBOX_MOD ((unsigned char)(DIM(sbox) - 1))
#define SWAP_ELEMENTS(array, i1, i2) { \
   unsigned char t; \
   t= array[i1]; array[i1]= array[i2]; array[i2]= t; \
}

static void a4_init(void) {
   unsigned i;
   for (i= (unsigned)DIM(sbox); i--; ) sbox[i]= (unsigned char)i;
}

static void a4_feed(char const *key_segment, size_t bytes) {
   unsigned i, j;
   size_t k;
   for (k= i= j= 0; k < bytes; ++k) {
      i= i + 1 & SBOX_MOD;
      j= j + sbox[i] + (unsigned char)key_segment[k] & SBOX_MOD;
      SWAP_ELEMENTS(sbox, i, j);
   }
}

static void a4_warmup(void) {
   unsigned i, j, k= 256 + 3 * (unsigned)DIM(sbox);
   i= j= 0;
   while (k--) {
      i= i + 1 & SBOX_MOD;
      j= j + sbox[i] & SBOX_MOD;
      SWAP_ELEMENTS(sbox, i, j);
   }
}

static void setup_pearson(void) {
   a4_init();
   {
      static const char personalizer[]=
         "Pearson hash seeded by ARCFOUR CSPRNG"
      ;
      a4_feed(personalizer, DIM(personalizer));
   }
   a4_warmup();
}

typedef struct {
   char *start;
   size_t length, capacity;
} bdesc;

typedef struct {
   bdesc b;
   r4g_action dtor, *saved;
} bdres;

static void bdesc_init_c0(bdesc *b) {
   b->start= 0; b->length= b->capacity= 0;
}

static void bdesc_resize_c1(bdesc *b, size_t required) {
   if (required) {
      size_t ncap= 128;
      void *new_p;
      assert(required >= b->length);
      required+= 80;
      while (ncap < required) ncap+= ncap;
      assert(!b->start == !b->capacity);
      if (!(new_p= realloc(b->start, ncap))) {
         error_c1(T("Memory allocation failure!"));
      }
      b->start= new_p;
      b->capacity= ncap;
   } else {
      void *p;
      if (p= b->start) {
         bdesc_init_c0(b);
         free(p);
      }
      assert(!b->start); assert(!b->length); assert(!b->capacity);
   }
}

static void bdres_dtor(void) {
   R4G_DEFINE_INIT_RPTR(bdres, *r=, dtor);
   r4g.rlist= r->saved;
   bdesc_resize_c1(&r->b, 0);
}

static void bdres_init_c4(bdres *r) {
   bdesc_init_c0(&r->b);
   r->saved= r4g.rlist;
   r->dtor= &bdres_dtor;
   r4g.rlist= &r->dtor;
}

/* Set the first unused buffer byte to a zero value and return a pointer to
 * the resulting buffer as a null-terminated string. Will grow the buffer if
 * no unused capacity bytes are available. */
static char *cstr_c1(bdesc *s) {
   if (s->capacity <= s->length) {
      assert(s->capacity == s->length);
      bdesc_resize_c1(s, s->length + 1);
      assert(s->length < s->capacity);
   }
   s->start[s->length]= '\0';
}

static int gets_c1(bdesc *b, size_t max_columns) {
   size_t i, n;
   int c;
   i= b->length= 0; n= b->capacity;
   while ((c= getchar()) != EOF) {
      if (c == '\n') goto enough;
      if (i >= max_columns) {
         error_c1(T("Input line too long!"));
      }
      if (i >= n) {
         bdesc_resize_c1(b, i + 1);
         n= b->capacity;
         assert(i < n);
      }
      b->start[i++]= (char)c;
   }
   if (ferror(stdin)) raise_read_error_c1();
   assert(feof(stdin));
   enough:
   b->length= i;
   (void)cstr_c1(b);
   return c != EOF || i;
}

static void index_stdin(void) {
   r4g_action *mark= r4g.rlist;
   bdres line;
   bdres_init_c4(&line);
   while (gets_c1(&line.b, 16384)) ;
   release_after_c1(mark);
}

int main(int argc, char **argv) {
   struct minimal_resource error_reporter;
   error_reporter.action= &error_reporter_action;
   error_reporter.saved= r4g.rlist;
   r4g.rlist= &error_reporter.action;
   if (argc > 1) {
      error_c1(
         T( \
            "Please feed the file listing to be indexed via" \
            " standard input without specifying any arguments!" \
            " If a line contains the string '///', then one the" \
            " text left of it will be indexed, and will be" \
            " replaced by the text right of it in the output." \
         )
      );
   }
   (void)argv;
   setup_pearson();
   index_stdin();
   done:
   fflush_c1(0);
   release_c1();
   return EXIT_SUCCESS;
}
