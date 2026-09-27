#include <typedefs.h>
typedef uint16 chanspec_t;
#define MCSSET_LEN	16	

#define	WLC_IOCTL_MAXLEN	8192		/* max length ioctl buffer required */
#define WL_NUMCHANSPECS		100
#define WL_SCAN_PARAMS_FIXED_SIZE 64
#define WLC_SCAN_RESULTS	 51


#define BIT(n) (1 << (n))

#define WPA_CIPHER_NONE_ BIT(0)
#define WPA_CIPHER_WEP40_ BIT(1)
#define WPA_CIPHER_WEP104_ BIT(2)
#define WPA_CIPHER_TKIP_ BIT(3)
#define WPA_CIPHER_CCMP_ BIT(4)

#define WPA_KEY_MGMT_IEEE8021X_ BIT(0)
#define WPA_KEY_MGMT_IEEE8021X2_ BIT(6)
#define WPA_KEY_MGMT_PSK_ BIT(1)
#define WPA_KEY_MGMT_PSK2_ BIT(5)
#define WPA_KEY_MGMT_NONE_ BIT(2)
#define WPA_KEY_MGMT_IEEE8021X_NO_WPA_ BIT(3)
#define WPA_KEY_MGMT_WPA_NONE_ BIT(4)
