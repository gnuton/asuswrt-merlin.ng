#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#define TYPEDEF_BOOL
#include <shared.h>
#include <shutils.h>
#include <bcmnvram.h>
#include <openvpn_config.h>
#include "openvpn_options.h"

char ignore_options[2048];

char* ovpn_client_option_list[] = {
	"local",
	"remote",
	"remote-random",
	"remote-random-hostname",
	"mode",
	"proto",
	"proto-force",
	"connect-retry",
	"connect-retry-max",
	"http-proxy",
	"http-proxy-user-pass",
	"http-proxy-option",
	"socks-proxy",
	"resolv-retry",
	"float",
	"ipchange",
	"port",
	"lport",
	"rport",
	"bind",
	"nobind",
	"dev",
	"dev-type",
	"dev-node",
	"lladdr",
	"topology",
	"ifconfig",
	"ifconfig-ipv6",
	"ifconfig-noexec",
	"ifconfig-nowarn",
	"route",
	"route-ipv6",
	"route-gateway",
	"route-metric",
	"route-delay",
	"route-up",
	"route-pre-down",
	"route-noexec",
	"route-up",
	"route-nopull",
	"allow-pull-fqdn",
	"ifconfig,",
	"redirect-gateway",
	"redirect-private",
	"client-nat",
	"push-peer-info",
	"setenv",
	"setenv-safe",
	"ignore-unknown-option",
	"script-security",
	"shaper",
	"keepalive",
	"inactive",
	"ping-exit",
	"ping-restart",
	"ping-timer-rem",
	"ping",
	"multihome",
	"fast-io",
	"remap-usr1",
	"persist-tun",
	"persist-remote-ip",
	"persist-local-ip",
	"persist-key",
	"passtos",
	"tun-mtu",
	"tun-mtu-extra",
	"link-mtu", "udp-mtu",
	"mtu-disc",
	"mtu-test",
	"fragment",
	"mssfix",
	"sndbuf",
	"rcvbuf",
	"mark",
	"socket-flags",
	"txqueuelen",
	"memstats",
	"mlock",
	"up",
	"up-delay",
	"down",
	"down-pre",
	"up-restart",
	"user",
	"group",
	"dhcp-option",
	"route-method",
	"chroot",
	"cd",
	"daemon",
	"syslog",
	"inetd",
	"log",
	"log-append",
	"suppress-timestamps",
	"machine-readable-output",
	"writepid",
	"nice",
	"echo",
	"parameter",
	"verb",
	"mute",
	"errors-to-stderr",
	"status",
	"status-version",
	"disable-occ",
	"compress",
	"comp-lzo",
	"comp-noadapt",
	"management",
	"management-client",
	"management-query-passwords",
	"management-query-proxy",
	"management-query-remote",
	"management-hold",
	"management-signal",
	"management-forget-disconnect",
	"management-up-down",
	"management-log-cache",
	"management-client-user",
	"management-client-group",
	"management-client-auth",
	"management-client-pf",
	"plugin",
	"client",
	"pull",
	"pull-filter",
	"push-continuation",
	"auth-user-pass",
	"auth-retry",
	"static-challenge",
	"connect-timeout", "server-poll-timeout",
	"allow-recursive-routing",
	"explicit-exit-notify",
	"key-direction",
	"secret",
	"auth",
	"cipher",
	"ncp-ciphers",
	"ncp-disable",
	"prng",
	"keysize",
	"ecdh-curves",
	"engine",
	"no-replay",
	"mute-replay-warnings",
	"replay-window",
	"no-iv",
	"replay-persist",
	"test-crypto",
	"tls-server",
	"tls-client",
	"key-method",
	"ca",
	"capath",
	"dh",
	"cert",
	"extra-certs",
	"key",
	"tls-version-min",
	"tls-version-max",
	"pkcs12",
	"verify-hash",
	"tls-cipher",
	"tls-cert-profile",
	"tls-ciphersuites",
	"tls-timeout",
	"reneg-bytes",
	"reneg-pkts",
	"reneg-sec",
	"hand-window",
	"tran-window",
	"single-session:",
	"tls-exit",
	"tls-auth",
	"tls-crypt",
	"askpass",
	"auth-nocache",
	"crl-verify",
	"tls-verify",
	"tls-export-cert",
	"verify-x509-name",
	"ns-cert-type",
	"x509-track",
	"keying-material-exporter",
	"remote-cert-ku",
	"remote-cert-eku",
	"remote-cert-tls",
	"pkcs11-providers",
	"pkcs11-protected-authentication",
	"pkcs11-private-mode",
	"pkcs11-cert-private",
	"pkcs11-pin-cache",
	"pkcs11-id-management",
	"pkcs11-id",
	NULL
};

struct buffer
alloc_buf (size_t size)
{
	struct buffer buf;

	if (!buf_size_valid (size))
		buf_size_error (size);
	buf.capacity = (int)size;
	buf.offset = 0;
	buf.len = 0;
	buf.data = calloc (1, size);

	return buf;
}

void
buf_size_error (const size_t size)
{
	logmessage ("OVPN", "fatal buffer size error, size=%lu", (unsigned long)size);
}

bool
buf_printf (struct buffer *buf, const char *format, ...)
{
	int ret = false;
	if (buf_defined (buf))
	{
		va_list arglist;
		uint8_t *ptr = buf_bend (buf);
		int cap = buf_forward_capacity (buf);

		if (cap > 0)
		{
			int stat;
			va_start (arglist, format);
			stat = vsnprintf ((char *)ptr, cap, format, arglist);
			va_end (arglist);
			*(buf->data + buf->capacity - 1) = 0; /* windows vsnprintf needs this */
			buf->len += (int) strlen ((char *)ptr);
			if (stat >= 0 && stat < cap)
				ret = true;
		}
	}
	return ret;
}

bool
buf_parse (struct buffer *buf, const int delim, char *line, const int size)
{
	bool eol = false;
	int n = 0;
	int c;

	do
	{
		c = buf_read_u8 (buf);
		if (c < 0)
			eol = true;
		if (c <= 0 || c == delim)
			c = 0;
		if (n >= size)
			break;
		line[n++] = c;
	}
	while (c);

	line[size-1] = '\0';
	return !(eol && !strlen (line));
}

void
buf_clear (struct buffer *buf)
{
	if (buf->capacity > 0)
		memset (buf->data, 0, buf->capacity);
	buf->len = 0;
	buf->offset = 0;
}

void
free_buf (struct buffer *buf)
{
	if (buf->data)
		free (buf->data);
	CLEAR (*buf);
}

char *
string_alloc (const char *str)
{
	if (str)
	{
		const int n = strlen (str) + 1;
		char *ret;

		ret = calloc(n, 1);
		if(!ret)
			return NULL;

		memcpy (ret, str, n);
		return ret;
	}
	else
		return NULL;
}

static inline bool
space (unsigned char c)
{
	return c == '\0' || isspace (c);
}

int
parse_line (const char *line, char *p[], const int n, const int line_num)
{
	const int STATE_INITIAL = 0;
	const int STATE_READING_QUOTED_PARM = 1;
	const int STATE_READING_UNQUOTED_PARM = 2;
	const int STATE_DONE = 3;
	const int STATE_READING_SQUOTED_PARM = 4;

	int ret = 0;
	const char *c = line;
	int state = STATE_INITIAL;
	bool backslash = false;
	char in, out;

	char parm[OPTION_PARM_SIZE];
	unsigned int parm_len = 0;

	do
	{
		in = *c;
		out = 0;

		if (!backslash && in == '\\' && state != STATE_READING_SQUOTED_PARM)
		{
			backslash = true;
		}
		else
		{
			if (state == STATE_INITIAL)
			{
				if (!space (in))
				{
					if (in == ';' || in == '#') /* comment */
						break;
					if (!backslash && in == '\"')
						state = STATE_READING_QUOTED_PARM;
					else if (!backslash && in == '\'')
						state = STATE_READING_SQUOTED_PARM;
					else
					{
						out = in;
						state = STATE_READING_UNQUOTED_PARM;
					}
				}
			}
			else if (state == STATE_READING_UNQUOTED_PARM)
			{
				if (!backslash && space (in))
					state = STATE_DONE;
				else
					out = in;
			}
			else if (state == STATE_READING_QUOTED_PARM)
			{
				if (!backslash && in == '\"')
					state = STATE_DONE;
				else
					out = in;
			}
			else if (state == STATE_READING_SQUOTED_PARM)
			{
				if (in == '\'')
					state = STATE_DONE;
				else
					out = in;
			}

			if (state == STATE_DONE)
			{
				p[ret] = calloc (parm_len + 1, 1);
				memcpy (p[ret], parm, parm_len);
				p[ret][parm_len] = '\0';
				state = STATE_INITIAL;
				parm_len = 0;
				++ret;
			}

			if (backslash && out)
			{
				if (!(out == '\\' || out == '\"' || space (out)))
				{
					logmessage ("OVPN", "Options warning: Bad backslash ('\\') usage in %d", line_num);
					return 0;
				}
			}
			backslash = false;
		}

		/* store parameter character */
		if (out)
		{
			if (parm_len >= SIZE (parm))
			{
				parm[SIZE (parm) - 1] = 0;
				logmessage ("OVPN", "Options error: Parameter at %d is too long (%d chars max): %s",
					line_num, (int) SIZE (parm), parm);
				return 0;
			}
			parm[parm_len++] = out;
		}

		/* avoid overflow if too many parms in one config file line */
		if (ret >= n)
			break;

	} while (*c++ != '\0');


	if (state == STATE_READING_QUOTED_PARM)
	{
		logmessage ("OVPN", "Options error: No closing quotation (\") in %d", line_num);
		return 0;
	}
	if (state == STATE_READING_SQUOTED_PARM)
	{
		logmessage ("OVPN", "Options error: No closing single quotation (\') in %d", line_num);
		return 0;
	}
	if (state != STATE_INITIAL)
	{
		logmessage ("OVPN", "Options error: Residual parse state (%d) in %d", state, line_num);
		return 0;
	}

	return ret;
}

static void
bypass_doubledash (char **p)
{
	if (strlen (*p) >= 3 && !strncmp (*p, "--", 2))
		*p += 2;
}

static bool
in_src_get (const struct in_src *is, char *line, const int size)
{
	if (is->type == IS_TYPE_FP)
	{
		return BOOL_CAST (fgets (line, size, is->u.fp));
	}
	else if (is->type == IS_TYPE_BUF)
	{
		bool status = buf_parse (is->u.multiline, '\n', line, size);
		if ((int) strlen (line) + 1 < size)
			strcat (line, "\n");
		return status;
	}
	else
	{
		return false;
	}
}

static char *
read_inline_file (struct in_src *is, const char *close_tag)
{
	char line[OPTION_LINE_SIZE];
	struct buffer buf = alloc_buf (10000);
	char *ret;
	while (in_src_get (is, line, sizeof (line)))
	{
		if (!strncmp (line, close_tag, strlen (close_tag)))
			break;
		buf_printf (&buf, "%s", line);
	}
	ret = string_alloc (buf_str (&buf));
	buf_clear (&buf);
	free_buf (&buf);
	CLEAR (line);
	return ret;
}

static bool
check_inline_file (struct in_src *is, char *p[])
{
	bool ret = false;
	if (p[0] && !p[1])
	{
		char *arg = p[0];
		if (arg[0] == '<' && arg[strlen(arg)-1] == '>')
		{
			struct buffer close_tag;
			arg[strlen(arg)-1] = '\0';
			p[0] = string_alloc (arg+1);
			p[1] = string_alloc (INLINE_FILE_TAG);
			close_tag = alloc_buf (strlen(p[0]) + 4);
			buf_printf (&close_tag, "</%s>", p[0]);
			p[2] = read_inline_file (is, buf_str (&close_tag));
			p[3] = NULL;
			free_buf (&close_tag);
			ret = true;
		}
	}
	return ret;
}

static bool
check_inline_file_via_fp (FILE *fp, char *p[])
{
	struct in_src is;
	is.type = IS_TYPE_FP;
	is.u.fp = fp;
	return check_inline_file (&is, p);
}

void
add_custom(int unit, char *p[])
{
	char nv[32];
	char *custom;
	char *param = NULL;
	char *final_custom = NULL;
	int i = 0, size = 0, sizeParam = 0;

	if(!p[0])
		return;

	while(p[i]) {
		size += strlen(p[i]) + 1;
		if(strchr(p[i], ' ')) {
			size += 2;
		}
		i++;
	}

	param = (char*)calloc(size, sizeof(char));
	sizeParam = size * sizeof(char);

	if(!param)
		return;

	i = 0;
	while(p[i]) {
		//_dprintf("p[%d]: [%s]\n", i, p[i]);
		if(*param)
			strlcat(param, " ", sizeParam);

		if(strchr(p[i], ' ')) {
			strlcat(param, "\"", sizeParam);
			strlcat(param, p[i], sizeParam);
			strlcat(param, "\"", sizeParam);
		}
		else {
			strlcat(param, p[i], sizeParam);
		}

		i++;
	}
	_dprintf("add [%s]\n", param);

	snprintf(nv, sizeof(nv), "vpn_client%d_custom", unit);
	custom = nvram_safe_get(nv);
	if(custom) {
		final_custom = calloc(strlen(custom) + strlen(param) + 2, sizeof(char));
		sizeParam = (strlen(custom) + strlen(param) + 2)*sizeof(char);
		if(final_custom) {
			if(*custom) {
				strlcat(final_custom, custom, sizeParam);
				strlcat(final_custom, "\n", sizeParam);
			}
			strlcat(final_custom, param, sizeParam);
			nvram_set(nv, final_custom);
			free(final_custom);
		}
	}
	else
		nvram_set(nv, param);

	free(param);
}

static int
add_option (char *p[], int line, int unit)
{
	char prefix[32] = {0};

	snprintf(prefix, sizeof(prefix), "vpn_client%d_", unit);

	if  (streq (p[0], "dev") && p[1])
	{
		if(!strncmp(p[1], "tun", 3))
			nvram_pf_set(prefix, "if", "tun");
		else if(!strncmp(p[1], "tap", 3))
			nvram_pf_set(prefix, "if", "tap");
	}
	else if  (streq (p[0], "proto") && p[1])
	{
		nvram_pf_set(prefix, "proto", p[1]);
	}
	else if  (streq (p[0], "remote") && p[1])
	{
		nvram_pf_set(prefix, "addr", p[1]);

		if(p[2])
			nvram_pf_set(prefix, "port", p[2]);

		if(p[3])
			nvram_pf_set(prefix, "proto", p[3]);
	}
	else if  (streq (p[0], "port") && p[1])
	{
		nvram_pf_set(prefix, "port", p[1]);
	}
	else if (streq (p[0], "resolv-retry") && p[1])
	{
		if (streq (p[1], "infinite"))
			nvram_pf_set(prefix, "retry", "-1");
		else
			nvram_pf_set(prefix, "retry", p[1]);
	}
	else if (streq (p[0], "comp-lzo"))
	{
		if(p[1])
			nvram_pf_set(prefix, "comp", p[1]);
		else
			nvram_pf_set(prefix, "comp", "adaptive");
	}
	else if (streq (p[0], "compress"))
	{
		if(p[1])
			nvram_pf_set(prefix, "comp", p[1]);
		else
			nvram_pf_set(prefix, "comp", "no");
	}
	else if (streq (p[0], "cipher") && p[1])
	{
		nvram_pf_set(prefix, "cipher", p[1]);
	}
	else if (streq (p[0], "auth") && p[1])
	{
		nvram_pf_set(prefix, "digest", p[1]);
	}
	else if (streq (p[0], "verb") && p[1])
	{
		nvram_pf_set(prefix, "verb", p[1]);
	}
	else if  (streq (p[0], "ca") && p[1])
	{
		nvram_pf_set(prefix, "crypt", "tls");
		if (streq (p[1], INLINE_FILE_TAG) && p[2])
		{
			set_ovpn_key(OVPN_TYPE_CLIENT, unit, OVPN_CLIENT_CA, p[2], NULL);
		}
		else
		{
			return VPN_UPLOAD_NEED_CA_CERT;
		}
	}
	else if  (streq (p[0], "cert") && p[1])
	{
		if (streq (p[1], INLINE_FILE_TAG) && p[2])
		{
			set_ovpn_key(OVPN_TYPE_CLIENT, unit, OVPN_CLIENT_CERT, p[2], NULL);
		}
		else
		{
			return VPN_UPLOAD_NEED_CERT;
		}
	}
	else if (streq (p[0], "extra-certs") && p[1])
	{
		if (streq (p[1], INLINE_FILE_TAG) && p[2])
		{
			set_ovpn_key(OVPN_TYPE_CLIENT, unit, OVPN_CLIENT_EXTRA, p[2], NULL);
		}
		else
		{
			return VPN_UPLOAD_NEED_EXTRA;
		}
	}
	else if  (streq (p[0], "key") && p[1])
	{
		if (streq (p[1], INLINE_FILE_TAG) && p[2])
		{
			set_ovpn_key(OVPN_TYPE_CLIENT, unit, OVPN_CLIENT_KEY, p[2], NULL);
		}
		else
		{
			return VPN_UPLOAD_NEED_KEY;
		}
	}
	else if (streq (p[0], "tls-auth") && p[1])
	{
		if (streq (p[1], INLINE_FILE_TAG) && p[2])
		{
			set_ovpn_key(OVPN_TYPE_CLIENT, unit, OVPN_CLIENT_STATIC, p[2], NULL);
			//key-direction
			if(nvram_pf_match(prefix, "hmac", "-1"))	//default, disable
				nvram_pf_set(prefix, "hmac", "2");	//openvpn default value: KEY_DIRECTION_BIDIRECTIONAL
		}
		else
		{
			if(p[2]) {
				nvram_pf_set(prefix, "hmac", p[2]);
			}
			return VPN_UPLOAD_NEED_STATIC;
		}
	}
	else if (streq (p[0], "tls-crypt") && p[1])
	{
		nvram_pf_set(prefix, "tlscrypt", "1");
		if (streq (p[1], INLINE_FILE_TAG) && p[2])
		{
			set_ovpn_key(OVPN_TYPE_CLIENT, unit, OVPN_CLIENT_STATIC, p[2], NULL);
		}
		else
		{
			return VPN_UPLOAD_NEED_STATIC;
		}
	}
	else if (streq (p[0], "secret") && p[1])
	{
		nvram_pf_set(prefix, "crypt", "secret");
		if (streq (p[1], INLINE_FILE_TAG) && p[2])
		{
			set_ovpn_key(OVPN_TYPE_CLIENT, unit, OVPN_CLIENT_STATIC, p[2], NULL);
		}
		else
		{
			return VPN_UPLOAD_NEED_STATIC;
		}
	}
	else if (streq (p[0], "auth-user-pass"))
	{
		nvram_pf_set(prefix, "userauth", "1");
	}
	else if (streq (p[0], "tls-remote") && p[1])
	{
		nvram_pf_set(prefix, "tlsremote", "1");
		nvram_pf_set(prefix, "cn", p[1]);
	}
	else if (streq (p[0], "verify-x509-name") && p[1] && p[2])
	{
		if (streq(p[2], "name"))
			nvram_pf_set(prefix, "tlsremote", "1");
		else if (streq(p[2], "name-prefix"))
			nvram_pf_set(prefix, "tlsremote", "2");
		else if (streq(p[2], "subject"))
			nvram_pf_set(prefix, "tlsremote", "3");
		nvram_pf_set(prefix, "cn", p[1]);
	}
	else if (streq (p[0], "key-direction") && p[1])
	{
		nvram_pf_set(prefix, "hmac", p[1]);
	}
	else if (streq (p[0], "crl-verify") && p[1])
	{
		if (p[2] && streq(p[2], "dir"))
			;//TODO: not support?
		return VPN_UPLOAD_NEED_CRL;
	}
	else if (streq(p[0], "ignore-unknown-option") && p[1])
	{
		int i;
		for (i = 1; p[i]; i++)
		{
			strlcat(ignore_options, p[i], sizeof(ignore_options));
			strlcat(ignore_options, " ", sizeof(ignore_options));
		}
	}
	else
	{
		if ( streq (p[0], "client")
			|| streq (p[0], "nobind")
			|| streq (p[0], "persist-key")
			|| streq (p[0], "persist-tun")
			|| streq (p[0], "user")
			|| streq (p[0], "group")
		) {
			;//ignore
		}
		else {
			add_custom(unit, p);
		}
	}
	return 0;
}

static int
check_valid_option(const char* data)
{
	int i = 0;

	if (!*data)
		return 0;

	while (ovpn_client_option_list[i]) {
		if (!strcmp(ovpn_client_option_list[i], data))
			return 1;
		i++;
	}
	if (ignore_options[0]) {
		char word[256] = {0};
		char *next = NULL;
		foreach(word, ignore_options, next) {
			if (!strcmp(word, data)) {
				return 2;
			}
		}
	}
	return 0;
}

int
read_config_file (const char *file, int unit)
{
	FILE *fp;
	int line_num;
	char line[OPTION_LINE_SIZE];
	char *p[MAX_PARMS];
	int ret = 0;
	char prefix[32] = {0};

	fp = fopen (file, "r");
	if (fp)
	{
		memset(ignore_options, 0 , sizeof(ignore_options));
		line_num = 0;
		while (fgets(line, sizeof (line), fp))
		{
			int offset = 0;
			int check_option_result = 0;
			CLEAR (p);
			++line_num;
			/* Ignore UTF-8 BOM at start of stream */
			if (line_num == 1 && strncmp (line, "\xEF\xBB\xBF", 3) == 0)
				offset = 3;
			if (parse_line (line + offset, p, SIZE (p), line_num))
			{
				bypass_doubledash (&p[0]);
				check_inline_file_via_fp (fp, p);
				if ((check_option_result = check_valid_option(p[0])) == 0)
				{
					_dprintf("Unrecoginzed or unsupported option: [%s]\n", p[0]);
					logmessage ("OVPN", "Unrecoginzed or unsupported option: [%s]", p[0]);
					ret = -1;
					break;
				}
				else if (check_option_result == 2)
				{
					_dprintf("Ignore unknown option: [%s]\n", p[0]);
					// logmessage ("OVPN", "Ignore unknown option: [%s]", p[0]);
				}
				else
				{
					ret |= add_option (p, line_num, unit);
				}
			}
		}
		fclose (fp);

		if( !(ret & VPN_UPLOAD_NEED_KEY)
			&& !ovpn_key_exists(OVPN_TYPE_CLIENT, unit, OVPN_CLIENT_KEY)
		) {
			snprintf(prefix, sizeof(prefix), "vpn_client%d_", unit);
			nvram_pf_set(prefix, "useronly", "1");
		}
	}
	else
	{
		logmessage ("OVPN", "Error opening configuration file");
	}

	CLEAR (line);
	CLEAR (p);

	return ret;
}

void parse_openvpn_status(int unit)
{
	FILE *fpi, *fpo;
	char buf[512];
	char *token;
	ovpn_sconf_t conf;
	char word[64] = {0};
	char *next = NULL;
	int i = 0, count = 0;
	int i_raddr = 0, i_vaddr = 0, i_user = 0;
	char raddr[64] = {0}, vaddr[64] = {0}, user[64] = {0};

	snprintf(buf, sizeof(buf), "/etc/openvpn/server%d/status", unit);
	fpi = fopen(buf, "r");

	snprintf(buf, sizeof(buf), "/etc/openvpn/server%d/client_status", unit);
	fpo = fopen(buf, "w");

	get_ovpn_sconf(unit, &conf);

	if(fpi && fpo) {
		while(!feof(fpi)){
			CLEAR(buf);
			if (!fgets(buf, sizeof(buf), fpi))
				break;
			if(!strncmp(buf, "HEADER,CLIENT_LIST", 18) && conf.auth_mode == OVPN_AUTH_TLS) {
				i = 0;
				foreach_44(word, buf, next) {
					if (!strcmp(word, "Real Address"))
						i_raddr = i - 1;
					else if (!strcmp(word, "Virtual Address"))
						i_vaddr = i - 1;
					else if (!strcmp(word, "Username"))
						i_user = i - 1;
					i++;
				}
			}
			else if(!strncmp(buf, "CLIENT_LIST", 11) && conf.auth_mode == OVPN_AUTH_TLS) {
				i = 0;
				foreach_44_keep_empty_string(count, word, buf, next) {
					if (i == i_raddr)
						strlcpy(raddr, word, sizeof(raddr));
					else if (i == i_vaddr)
						strlcpy(vaddr, word, sizeof(vaddr));
					else if (i == i_user)
						strlcpy(user, word, sizeof(user));
					i++;
				}

				//Real Address
				if (raddr[0] == '\0')
					fprintf(fpo, "NoRealAddress ");
				else
					fprintf(fpo, "%s ", raddr);

				//Virtual Address
				if( conf.if_type == OVPN_IF_TAP && conf.dhcp == 1) {
					fprintf(fpo, "VirtualAddressAssignedByDhcp ");
				}
				else {
					if (vaddr[0] == '\0')
						fprintf(fpo, "NoVirtualAddress ");
					else
						fprintf(fpo, "%s ", vaddr);
				}

				//Username
				if (user[0] == '\0')
					fprintf(fpo, "NoUsername");
				else
					fprintf(fpo, "%s", user);

				fprintf(fpo, "\n");
			}
			else if(!strncmp(buf, "REMOTE", 6) && conf.auth_mode == OVPN_AUTH_STATIC) {
				token = strtok(buf, ",");	//REMOTE,
				token = strtok(NULL, ",");	//Real Address
				if(token)
					fprintf(fpo, "%s ", token);
				else
					fprintf(fpo, "NoRealAddress ");

				fprintf(fpo, "%s ", conf.remote);
				fprintf(fpo, "Static_Key");
				break;
			}
		}
	}
	if(fpi) fclose(fpi);
	if(fpo) fclose(fpo);
}
