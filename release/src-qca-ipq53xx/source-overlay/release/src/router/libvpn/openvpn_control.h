extern void append_ovpn_accnt(const char *path, const char *ovpn_path);
extern void create_ovpn_passwd();
extern void run_ovpn_fw_scripts();
extern void run_ovpn_fw_nat_scripts();
extern void start_ovpn_eas();
extern void start_ovpn_client(int unit);
extern void start_ovpn_server(int unit);
extern void start_ovpn_clientall();
extern void start_ovpn_serverall();
extern void stop_ovpn_eas();
extern void stop_ovpn_client(int unit);
extern void stop_ovpn_server(int unit);
extern void stop_ovpn_clientall();
extern void stop_ovpn_serverall();
extern void write_ovpn_dnsmasq_config(FILE* f);
extern int write_ovpn_resolv_dnsmasq(FILE* fp_servers);
extern void update_ovpn_profie_remote();
extern void ovpn_up_handler();
extern void ovpn_down_handler();
extern void ovpn_route_up_handler();
extern void ovpn_route_pre_down_handler();
#ifdef RTCONFIG_MULTILAN_CFG
extern void update_ovpn_client_by_sdn(MTLAN_T *pmtl, size_t mtl_sz, int restart_all_sdn);
extern void update_ovpn_server_by_sdn(MTLAN_T *pmtl, size_t mtl_sz, int restart_all_sdn);
extern void update_ovpn_client_by_sdn_remove(MTLAN_T *pmtl, size_t mtl_sz);
extern void update_ovpn_server_by_sdn_remove(MTLAN_T *pmtl, size_t mtl_sz);
#endif
