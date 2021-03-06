/* SPDX-License-Identifier: LGPL-2.1+ */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <langinfo.h>
#include <libintl.h>
#include <locale.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "def.h"
#include "dirent-util.h"
#include "env-util.h"
#include "fd-util.h"
#include "hashmap.h"
#include "locale-util.h"
#include "path-util.h"
#include "set.h"
#include "string-table.h"
#include "string-util.h"
#include "strv.h"
#include "utf8.h"

static int add_locales_from_archive(Set *locales) {
        /* Stolen from glibc... */

        struct locarhead {
                uint32_t magic;
                /* Serial number.  */
                uint32_t serial;
                /* Name hash table.  */
                uint32_t namehash_offset;
                uint32_t namehash_used;
                uint32_t namehash_size;
                /* String table.  */
                uint32_t string_offset;
                uint32_t string_used;
                uint32_t string_size;
                /* Table with locale records.  */
                uint32_t locrectab_offset;
                uint32_t locrectab_used;
                uint32_t locrectab_size;
                /* MD5 sum hash table.  */
                uint32_t sumhash_offset;
                uint32_t sumhash_used;
                uint32_t sumhash_size;
        };

        struct namehashent {
                /* Hash value of the name.  */
                uint32_t hashval;
                /* Offset of the name in the string table.  */
                uint32_t name_offset;
                /* Offset of the locale record.  */
                uint32_t locrec_offset;
        };

        const struct locarhead *h;
        const struct namehashent *e;
        const void *p = MAP_FAILED;
        _cleanup_close_ int fd = -1;
        size_t sz = 0;
        struct stat st;
        size_t i;
        int r;

        fd = open("/usr/lib/locale/locale-archive", O_RDONLY|O_NOCTTY|O_CLOEXEC);
        if (fd < 0)
                return errno == ENOENT ? 0 : -errno;

        if (fstat(fd, &st) < 0)
                return -errno;

        if (!S_ISREG(st.st_mode))
                return -EBADMSG;

        if (st.st_size < (off_t) sizeof(struct locarhead))
                return -EBADMSG;

        p = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
        if (p == MAP_FAILED)
                return -errno;

        h = (const struct locarhead *) p;
        if (h->magic != 0xde020109 ||
            h->namehash_offset + h->namehash_size > st.st_size ||
            h->string_offset + h->string_size > st.st_size ||
            h->locrectab_offset + h->locrectab_size > st.st_size ||
            h->sumhash_offset + h->sumhash_size > st.st_size) {
                r = -EBADMSG;
                goto finish;
        }

        e = (const struct namehashent*) ((const uint8_t*) p + h->namehash_offset);
        for (i = 0; i < h->namehash_size; i++) {
                char *z;

                if (e[i].locrec_offset == 0)
                        continue;

                if (!utf8_is_valid((char*) p + e[i].name_offset))
                        continue;

                z = strdup((char*) p + e[i].name_offset);
                if (!z) {
                        r = -ENOMEM;
                        goto finish;
                }

                r = set_consume(locales, z);
                if (r < 0)
                        goto finish;
        }

        r = 0;

 finish:
        if (p != MAP_FAILED)
                munmap((void*) p, sz);

        return r;
}

static int add_locales_from_libdir (Set *locales) {
        _cleanup_closedir_ DIR *dir = NULL;
        struct dirent *entry;
        int r;

        dir = opendir("/usr/lib/locale");
        if (!dir)
                return errno == ENOENT ? 0 : -errno;

        FOREACH_DIRENT(entry, dir, return -errno) {
                char *z;

                dirent_ensure_type(dir, entry);

                if (entry->d_type != DT_DIR)
                        continue;

                z = strdup(entry->d_name);
                if (!z)
                        return -ENOMEM;

                r = set_consume(locales, z);
                if (r < 0 && r != -EEXIST)
                        return r;
        }

        return 0;
}

int get_locales(char ***ret) {
        _cleanup_set_free_ Set *locales = NULL;
        _cleanup_strv_free_ char **l = NULL;
        int r;

        locales = set_new(&string_hash_ops);
        if (!locales)
                return -ENOMEM;

        r = add_locales_from_archive(locales);
        if (r < 0 && r != -ENOENT)
                return r;

        r = add_locales_from_libdir(locales);
        if (r < 0)
                return r;

        l = set_get_strv(locales);
        if (!l)
                return -ENOMEM;

        strv_sort(l);

        *ret = TAKE_PTR(l);

        return 0;
}

bool locale_is_valid(const char *name) {

        if (isempty(name))
                return false;

        if (strlen(name) >= 128)
                return false;

        if (!utf8_is_valid(name))
                return false;

        if (!filename_is_valid(name))
                return false;

        if (!string_is_safe(name))
                return false;

        return true;
}

void init_gettext(void) {
        setlocale(LC_ALL, "");
        textdomain(GETTEXT_PACKAGE);
}

bool is_locale_utf8(void) {
        const char *set;
        static int cached_answer = -1;

        /* Note that we default to 'true' here, since today UTF8 is
         * pretty much supported everywhere. */

        if (cached_answer >= 0)
                goto out;

        if (!setlocale(LC_ALL, "")) {
                cached_answer = true;
                goto out;
        }

        set = nl_langinfo(CODESET);
        if (!set) {
                cached_answer = true;
                goto out;
        }

        if (streq(set, "UTF-8")) {
                cached_answer = true;
                goto out;
        }

        /* For LC_CTYPE=="C" return true, because CTYPE is effectly
         * unset and everything can do to UTF-8 nowadays. */
        set = setlocale(LC_CTYPE, NULL);
        if (!set) {
                cached_answer = true;
                goto out;
        }

        /* Check result, but ignore the result if C was set
         * explicitly. */
        cached_answer =
                STR_IN_SET(set, "C", "POSIX") &&
                !getenv("LC_ALL") &&
                !getenv("LC_CTYPE") &&
                !getenv("LANG");

out:
        return (bool) cached_answer;
}

static thread_local Set *keymaps = NULL;

static int nftw_cb(
                const char *fpath,
                const struct stat *sb,
                int tflag,
                struct FTW *ftwbuf) {

        char *p, *e;
        int r;

        if (tflag != FTW_F)
                return 0;

        if (!endswith(fpath, ".map") &&
            !endswith(fpath, ".map.gz"))
                return 0;

        p = strdup(basename(fpath));
        if (!p)
                return FTW_STOP;

        e = endswith(p, ".map");
        if (e)
                *e = 0;

        e = endswith(p, ".map.gz");
        if (e)
                *e = 0;

        r = set_consume(keymaps, p);
        if (r < 0 && r != -EEXIST)
                return r;

        return 0;
}

int get_keymaps(char ***ret) {
        _cleanup_strv_free_ char **l = NULL;
        const char *dir;
        int r;

        keymaps = set_new(&string_hash_ops);
        if (!keymaps)
                return -ENOMEM;

        NULSTR_FOREACH(dir, KBD_KEYMAP_DIRS) {
                r = nftw(dir, nftw_cb, 20, FTW_PHYS|FTW_ACTIONRETVAL);

                if (r == FTW_STOP)
                        log_debug("Directory not found %s", dir);
                else if (r < 0)
                        log_debug_errno(r, "Can't add keymap: %m");
        }

        l = set_get_strv(keymaps);
        if (!l) {
                set_free_free(keymaps);
                return -ENOMEM;
        }

        set_free(keymaps);

        if (strv_isempty(l))
                return -ENOENT;

        strv_sort(l);

        *ret = TAKE_PTR(l);

        return 0;
}

bool keymap_is_valid(const char *name) {

        if (isempty(name))
                return false;

        if (strlen(name) >= 128)
                return false;

        if (!utf8_is_valid(name))
                return false;

        if (!filename_is_valid(name))
                return false;

        if (!string_is_safe(name))
                return false;

        return true;
}

static bool emoji_enabled(void) {
        static int cached_emoji_enabled = -1;

        if (cached_emoji_enabled < 0) {
                int val;

                val = getenv_bool("SYSTEMD_EMOJI");
                if (val < 0)
                        cached_emoji_enabled =
                                is_locale_utf8() &&
                                !STRPTR_IN_SET(getenv("TERM"), "dumb", "linux");
                else
                        cached_emoji_enabled = val;
        }

        return cached_emoji_enabled;
}

const char *special_glyph(SpecialGlyph code) {

        /* A list of a number of interesting unicode glyphs we can use to decorate our output. It's probably wise to be
         * conservative here, and primarily stick to the glyphs defined in the eurlatgr font, so that display still
         * works reasonably well on the Linux console. For details see:
         *
         * http://git.altlinux.org/people/legion/packages/kbd.git?p=kbd.git;a=blob;f=data/consolefonts/README.eurlatgr
         */

        static const char* const draw_table[2][_SPECIAL_GLYPH_MAX] = {
                /* ASCII fallback */
                [false] = {
                        [SPECIAL_GLYPH_TREE_VERTICAL]           = "| ",
                        [SPECIAL_GLYPH_TREE_BRANCH]             = "|-",
                        [SPECIAL_GLYPH_TREE_RIGHT]              = "`-",
                        [SPECIAL_GLYPH_TREE_SPACE]              = "  ",
                        [SPECIAL_GLYPH_TRIANGULAR_BULLET]       = ">",
                        [SPECIAL_GLYPH_BLACK_CIRCLE]            = "*",
                        [SPECIAL_GLYPH_BULLET]                  = "*",
                        [SPECIAL_GLYPH_ARROW]                   = "->",
                        [SPECIAL_GLYPH_MDASH]                   = "-",
                        [SPECIAL_GLYPH_ELLIPSIS]                = "...",
                        [SPECIAL_GLYPH_MU]                      = "u",
                        [SPECIAL_GLYPH_CHECK_MARK]              = "+",
                        [SPECIAL_GLYPH_CROSS_MARK]              = "-",
                        [SPECIAL_GLYPH_ECSTATIC_SMILEY]         = ":-]",
                        [SPECIAL_GLYPH_HAPPY_SMILEY]            = ":-}",
                        [SPECIAL_GLYPH_SLIGHTLY_HAPPY_SMILEY]   = ":-)",
                        [SPECIAL_GLYPH_NEUTRAL_SMILEY]          = ":-|",
                        [SPECIAL_GLYPH_SLIGHTLY_UNHAPPY_SMILEY] = ":-(",
                        [SPECIAL_GLYPH_UNHAPPY_SMILEY]          = ":-{???",
                        [SPECIAL_GLYPH_DEPRESSED_SMILEY]        = ":-[",
                },

                /* UTF-8 */
                [true] = {
                        [SPECIAL_GLYPH_TREE_VERTICAL]           = "\342\224\202 ",            /* ???  */
                        [SPECIAL_GLYPH_TREE_BRANCH]             = "\342\224\234\342\224\200", /* ?????? */
                        [SPECIAL_GLYPH_TREE_RIGHT]              = "\342\224\224\342\224\200", /* ?????? */
                        [SPECIAL_GLYPH_TREE_SPACE]              = "  ",                       /*    */
                        [SPECIAL_GLYPH_TRIANGULAR_BULLET]       = "\342\200\243",             /* ??? */
                        [SPECIAL_GLYPH_BLACK_CIRCLE]            = "\342\227\217",             /* ??? */
                        [SPECIAL_GLYPH_BULLET]                  = "\342\200\242",             /* ??? */
                        [SPECIAL_GLYPH_ARROW]                   = "\342\206\222",             /* ??? */
                        [SPECIAL_GLYPH_MDASH]                   = "\342\200\223",             /* ??? */
                        [SPECIAL_GLYPH_ELLIPSIS]                = "\342\200\246",             /* ??? */
                        [SPECIAL_GLYPH_MU]                      = "\316\274",                 /* ?? */
                        [SPECIAL_GLYPH_CHECK_MARK]              = "\342\234\223",             /* ??? */
                        [SPECIAL_GLYPH_CROSS_MARK]              = "\342\234\227",             /* ??? */
                        [SPECIAL_GLYPH_ECSTATIC_SMILEY]         = "\360\237\230\207",         /* ???? */
                        [SPECIAL_GLYPH_HAPPY_SMILEY]            = "\360\237\230\200",         /* ???? */
                        [SPECIAL_GLYPH_SLIGHTLY_HAPPY_SMILEY]   = "\360\237\231\202",         /* ???? */
                        [SPECIAL_GLYPH_NEUTRAL_SMILEY]          = "\360\237\230\220",         /* ???? */
                        [SPECIAL_GLYPH_SLIGHTLY_UNHAPPY_SMILEY] = "\360\237\231\201",         /* ???? */
                        [SPECIAL_GLYPH_UNHAPPY_SMILEY]          = "\360\237\230\250",         /* ?????????? */
                        [SPECIAL_GLYPH_DEPRESSED_SMILEY]        = "\360\237\244\242",         /* ???? */
                },
        };

        assert(code < _SPECIAL_GLYPH_MAX);

        return draw_table[code >= _SPECIAL_GLYPH_FIRST_SMILEY ? emoji_enabled() : is_locale_utf8()][code];
}

void locale_variables_free(char *l[_VARIABLE_LC_MAX]) {
        LocaleVariable i;

        if (!l)
                return;

        for (i = 0; i < _VARIABLE_LC_MAX; i++)
                l[i] = mfree(l[i]);
}

static const char * const locale_variable_table[_VARIABLE_LC_MAX] = {
        [VARIABLE_LANG] = "LANG",
        [VARIABLE_LANGUAGE] = "LANGUAGE",
        [VARIABLE_LC_CTYPE] = "LC_CTYPE",
        [VARIABLE_LC_NUMERIC] = "LC_NUMERIC",
        [VARIABLE_LC_TIME] = "LC_TIME",
        [VARIABLE_LC_COLLATE] = "LC_COLLATE",
        [VARIABLE_LC_MONETARY] = "LC_MONETARY",
        [VARIABLE_LC_MESSAGES] = "LC_MESSAGES",
        [VARIABLE_LC_PAPER] = "LC_PAPER",
        [VARIABLE_LC_NAME] = "LC_NAME",
        [VARIABLE_LC_ADDRESS] = "LC_ADDRESS",
        [VARIABLE_LC_TELEPHONE] = "LC_TELEPHONE",
        [VARIABLE_LC_MEASUREMENT] = "LC_MEASUREMENT",
        [VARIABLE_LC_IDENTIFICATION] = "LC_IDENTIFICATION"
};

DEFINE_STRING_TABLE_LOOKUP(locale_variable, LocaleVariable);
