#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define ADD 0
#define SUB 1
#define LFT 2
#define RGT 3
#define JPZ 4
#define JNZ 5
#define GET 6
#define PUT 7
#define FLAG_DEBUG 1
#define FLAG_EXIT_ON_EOF 2

uint16_t expectany(char* s, char c) {
  uint16_t count = 0;
  while (*s++ == c)
    count++;
  return count;
}

void compile(char* source, uint16_t* bc) {
  uint16_t* stack[64];
  int depth = 0;
  uint16_t* base = bc;
  uint16_t* target;
  uint16_t val;
  for (; *source; source++) {
    switch (*source) {
      case 0:
      case '+':
        val = expectany(source, '+');
        *bc++ = (val << 3) | ADD;
        source += val - 1;
        break;
      case '-':
        val = expectany(source, '-');
        *bc++ = (val << 3) | SUB;
        source += val - 1;
        break;
      case '<':
        val = expectany(source, '<');
        *bc++ = (val << 3) | LFT;
        source += val - 1;
        break;
      case '>':
        val = expectany(source, '>');
        *bc++ = (val << 3) | RGT;
        source += val - 1;
        break;
      case '[':
        stack[depth++] = bc;
        *bc++ = 0;
        break;
      case ']':
        if (depth == 0) {
          *bc++ = 0;
          break;
        }
        target = stack[--depth];
        *target = ((bc - base + 1) << 3) | JPZ;
        *bc++ = ((target - base) << 3) | JNZ;
        break;
      case ',':
        *bc++ = GET;
        break;
      case '.':
        *bc++ = PUT;
        break;
    }
  }
  *bc = 0;
}

void disasm1(uint16_t insn) {
  static char* names[] = {
      "ADD", "SUB", "LFT", "RGT", "JPZ", "JNZ", "GET", "PUT",
  };
  printf("%04x %s %d\n", insn, names[insn & 7], insn >> 3);
}

void disasm(uint16_t* bc) {
  for (; *bc; bc++)
    disasm1(*bc);
}

void execute(uint16_t* bc, FILE* in, FILE* out, int flags) {
  unsigned char memory[UINT16_MAX + 1];
  uint16_t i = 0;
  uint16_t* pc = bc;
  int ch;
  memset(memory, 0, sizeof(memory));
  for (; *pc; pc++) {
    switch (*pc & 7) {
      case ADD:
        memory[i] += (*pc >> 3);
        break;
      case SUB:
        memory[i] -= (*pc >> 3);
        break;
      case LFT:
        i -= (*pc >> 3);
        break;
      case RGT:
        i += (*pc >> 3);
        break;
      case JPZ:
        if (memory[i] == 0)
          pc = bc + (*pc >> 3) - 1;
        break;
      case JNZ:
        if (memory[i] != 0)
          pc = bc + (*pc >> 3) - 1;
        break;
      case GET:
        ch = fgetc(in);
        if (ch == EOF) {
          if (flags & FLAG_EXIT_ON_EOF)
            return;
          ch = 0;
        }
        memory[i] = ch;
        break;
      case PUT:
        fputc(memory[i], out);
        break;
    }
  }
}

int usage(char** argv) {
  printf("Usage: %s [-d] [-x] [-i path] [-c 'program'] [-f path]\n", argv[0]);
  printf(" -d: prints disassembly of the bytecode\n");
  printf(" -x: exit on EOF, needed for some bf scripts to stop\n");
  printf(" -i: input file if not stdin\n");
  printf(" -c: execute bf program passed on the command line\n");
  printf(" -f: path to bf source file to execute\n");
  return EXIT_FAILURE;
}

int main(int argc, char** argv) {
  struct stat finfo;
  char* source;
  uint16_t* bc;
  int opt;
  int flags = 0;
  FILE* f;
  FILE* input = stdin;
  if (argc == 1) {
    return usage(argv);
  }
  while ((opt = getopt(argc, argv, "c:df:i:x")) != -1) {
    switch (opt) {
      case 'c':
        source = optarg;
        break;
      case 'd':
        flags |= FLAG_DEBUG;
        break;
      case 'x':
        flags |= FLAG_EXIT_ON_EOF;
        break;
      case 'f':
        f = fopen(optarg, "rb");
        if (f == NULL) {
          fprintf(stderr, "%s not found\n", optarg);
          return EXIT_FAILURE;
        }
        fstat(fileno(f), &finfo);
        source = malloc(finfo.st_size + 1);
        memset(source, 0, finfo.st_size);
        fread(source, finfo.st_size, 1, f);
        fclose(f);
        break;
      case 'i':
        if (strcmp(optarg, "-") != 0) {
          input = fopen(optarg, "rb");
          if (input == NULL) {
            fprintf(stderr, "%s not found\n", optarg);
            return EXIT_FAILURE;
          }
        }
        break;
      default:
        return usage(argv);
    }
  }
  bc = malloc(sizeof(uint16_t) * (strlen(source) + 1));
  compile(source, bc);
  if (flags & FLAG_DEBUG) {
    disasm(bc);
    return 0;
  }
  execute(bc, input, stdout, flags);
  return 0;
}
