#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define eprint(fmt, ...)   (fprintf(stderr, fmt     , ##__VA_ARGS__))
#define eprintln(fmt, ...) (fprintf(stderr, fmt "\n", ##__VA_ARGS__))
#define assert(x, ...)     ((x) ? 0 : (eprintln(__VA_ARGS__), exit(255)))

static int hlsym_intnlen_upto(const int *s, int len, int reject)
{
    const int *start = s;
    while (s < start + len && s[0] != reject) s += 1;
    return s - start;
}

static int hlsym_is_tagged(int pixel)
{
    return pixel >> 31;
}

static int hlsym_tag(int *artbuf, int artlen)
{
    int maxlinelen = 0;
    for (int *p = artbuf; p < artbuf + artlen; p += 1) {
        int linelen = hlsym_intnlen_upto(p, artbuf + artlen - p, '\n');
        if (linelen > maxlinelen) maxlinelen = linelen;
        p += linelen;
        if (p[0] == '\n') p += 1;
    }
    int has_tagged = 0;
    int m = maxlinelen / 2;
    while (artlen > 0) {
        for (int i = 0; i < m; i += 1) {
            int *lhs = &artbuf[m - i - 1];
            int *rhs = &artbuf[m + i + (m & 1)];
            if (rhs[0] == '\n')
                break;
            if (lhs[0] != rhs[0]) {
                has_tagged = 1;
                lhs[0] |= 1 << 31;
                rhs[0] |= 1 << 31;
            }
        }
        int linelen = hlsym_intnlen_upto(artbuf, artlen, '\n');
        artbuf += linelen;
        artlen -= linelen;
        if (artbuf[0] == '\n') {
            artbuf += 1;
            artlen -= 1;
        }
    }
    return has_tagged;
}

static void exit_error_with_path(const char *path)
{
    eprintln("%s: error: %s", path, strerror(errno));
    exit(1);
}

int main(int argc, char **argv)
{
    const char *invokpath = argv[0];
    assert(invokpath, "program was invoked with an empty argument list");

    int *artbuf = 0;
    int  artlen = 0;

    const char *artpath = (++argv)[0];
    if (artpath) {
        FILE *artstream = fopen(artpath, "rb");
        if (artstream == 0)                    exit_error_with_path(artpath);
        if (fseek(artstream, 0, SEEK_END) < 0) exit_error_with_path(artpath);
        if ((artlen = ftell(artstream)) < 0)   exit_error_with_path(artpath);
        if (fseek(artstream, 0, SEEK_SET) < 0) exit_error_with_path(artpath);
        if ((artbuf = malloc(sizeof(artbuf[0]) * artlen)) == 0)
            exit_error_with_path(artpath);
        for (int i = 0; i < artlen; i += 1) {
            fread(&artbuf[i], 1, 1, artstream);
            if (ferror(artstream)) exit_error_with_path(artpath);
        }
    } else {
        if ((artbuf = malloc(4096)) == 0) exit_error_with_path("<stdin>");
        for (; artlen < 4096; artlen += 1) {
            if (fread(&artbuf[artlen], 1, 1, stdin) == 0) {
                if (ferror(stdin)) exit_error_with_path("<stdin>");
                break;
            }
        }
    }

    hlsym_tag(artbuf, artlen);

    eprintln("======");
    for (int i = 0; i < artlen; i += 1) {
        if (hlsym_is_tagged(artbuf[i])) eprint("\033[91m");
        eprint("%c", artbuf[i]);
        if (hlsym_is_tagged(artbuf[i])) eprint("\033[0m");
    }
    eprintln("======");
}
