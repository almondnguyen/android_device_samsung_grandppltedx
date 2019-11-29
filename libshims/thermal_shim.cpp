
extern "C" {
	int ifc_set_throttle(const char * /*ifname*/, int /*rxKbps*/, int /*txKbps*/) { return 0; }

	int ifc_is_up(const char *, unsigned *) { return 0; };
	int ifc_enable_allmc(const char *) { return 0; };
	int ifc_disable_allmc(const char *){ return 0; };
	int ifc_reset_connection_by_uid(int, int){ return 0; };
	int ifc_set_fwmark_rule(const char *, int, int){ return 0; };
	int ifc_set_txq_state(const char *, int){ return 0; };
	int ifc_ccmni_md_cfg(const char *, int){ return 0; };

	struct uid_err {
		int appuid;
		int errorNum;
	};
}


