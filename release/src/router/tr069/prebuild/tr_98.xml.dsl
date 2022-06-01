<?xml version="1.0" encoding="UTF-8"?>
<node name='InternetGatewayDevice' rw='0' type='node' arg='1.1'>
	<node acl='' getc='0' name='DeviceSummary' noc='0' nocc='' rw='0' type='string'>ADSL/VDSL Modem Router</node>
	<node acl='' getc='0' name='LANDeviceNumberOfEntries' noc='0' nocc='' rw='0' type='unsignedInt'>1</node>
	<node acl='' getc='0' name='WANDeviceNumberOfEntries' noc='0' nocc='' rw='0' type='unsignedInt'>1</node>
	<node name='Capabilities' rw='0' type='node'>
		<node name='PerformanceDiagnostic' rw='0' type='node'>
			<node acl='' getc='0' name='DownloadTransports' noc='0' nocc='' rw='0' type='string'>HTTP</node>
			<node acl='' getc='0' name='UploadTransports' noc='0' nocc='' rw='0' type='string'>HTTP</node>
		</node>
	</node>
	<node name='DeviceInfo' rw='0' type='node'>
		<node acl='' getc='0' name='Manufacturer' noc='0' nocc='' rw='0' type='string'>ASUSTeK Computer Inc.</node>
		<node acl='' getc='0' name='ManufacturerOUI' noc='0' nocc='' rw='0' type='string' cmd='oui'></node>
		<node acl='' getc='0' name='ModelName' noc='0' nocc='' rw='0' type='string' cmd='nvram' arg='productid'></node>
		<node acl='' getc='0' name='ProductClass' noc='0' nocc='' rw='0' type='string' cmd='nvram' arg='productid'></node>
		<node acl='' getc='0' name='SerialNumber' noc='0' nocc='' rw='0' type='string' cmd='serial'></node>
		<node acl='' getc='0' name='HardwareVersion' noc='0' nocc='' rw='0' type='string'>1.0</node>
		<node acl='' getc='0' name='SoftwareVersion' noc='0' nocc='' rw='0' type='string' cmd='firmver'></node>
		<node acl='' getc='0' name='SpecVersion' noc='0' nocc='' rw='0' type='string'>1.0</node>
		<node acl='' getc='0' name='ProvisioningCode' noc='0' nocc='' rw='1' type='string' cmd='nvram' arg='pvgcode'></node>
		<node acl='' getc='0' name='UpTime' noc='0' nocc='' rw='0' type='unsignedInt' cmd='uptime'></node>
		<node acl='' getc='0' name='VendorConfigFileNumberOfEntries' noc='0' nocc='' rw='0' type='unsignedInt'>0</node>
		<node il='32' name='VendorConfigFile' nin='1' rw='1' type='node'>
			<node name='template' rw='1' type='node'>
				<node acl='' getc='0' name='Name' noc='0' nocc='' rw='0' type='string'></node>
				<node acl='' getc='0' name='Date' noc='0' nocc='' rw='0' type='dateTime'></node>
			</node>
		</node>
	</node>
	<node name='ManagementServer' rw='0' type='node'>
		<node acl='' getc='0' name='EnableCWMP' noc='0' nocc='' rw='1' type='boolean' cmd='nvram' arg='tr_enable'></node>
		<node acl='' getc='0' name='URL' noc='0' nocc='' rw='1' type='string' cmd='nvram' arg='tr_acs_url'></node>
		<node acl='' getc='0' name='Username' noc='0' nocc='' rw='1' type='string' cmd='nvram' arg='tr_username'></node>
		<node acl='' getc='1' name='Password' noc='0' nocc='' rw='1' type='string' cmd='nvram' arg='tr_passwd'></node>
		<node acl='' getc='0' name='PeriodicInformEnable' noc='0' nocc='' rw='1' type='boolean' cmd='nvram' arg='tr_inform_enable'></node>
		<node acl='' getc='0' name='PeriodicInformInterval' noc='0' nocc='' rw='1' type='unsignedInt' cmd='nvram' arg='tr_inform_interval'></node>
		<node acl='' getc='0' name='PeriodicInformTime' noc='0' nocc='' rw='1' type='dateTime'>0000-00-00T00:00:00</node>
		<node acl='' getc='0' name='ParameterKey' noc='0' nocc='' rw='0' type='string'></node>
		<node acl='' getc='0' name='ConnectionRequestURL' noc='0' nocc='' rw='0' type='string' cmd='conn_url'></node>
		<node acl='' getc='0' name='ConnectionRequestUsername' noc='0' nocc='' rw='1' type='string' cmd='nvram' arg='tr_conn_username'></node>
		<node acl='' getc='1' name='ConnectionRequestPassword' noc='0' nocc='' rw='1' type='string' cmd='nvram' arg='tr_conn_passwd'></node>
		<node acl='' getc='0' name='UDPConnectionRequestAddress' noc='0' nocc='' rw='0' type='string'></node>
		<node acl='' getc='0' name='UDPConnectionRequestAddressNotificationLimit' noc='0' nocc='' rw='1' type='unsignedInt'></node>
		<node acl='' getc='0' name='STUNEnable' noc='0' nocc='' rw='1' type='boolean'>false</node>
		<node acl='' getc='0' name='STUNServerAddress' noc='0' nocc='' rw='1' type='string'></node>
		<node acl='' getc='0' name='STUNServerPort' noc='0' nocc='' rw='1' type='unsignedInt'></node>
		<node acl='' getc='0' name='STUNUsername' noc='0' nocc='' rw='1' type='string'></node>
		<node acl='' getc='1' name='STUNPassword' noc='0' nocc='' rw='1' type='string'></node>
		<node acl='' getc='0' name='STUNMaximumKeepAlivePeriod' noc='0' nocc='' rw='1' type='int'></node>
		<node acl='' getc='0' name='STUNMinimumKeepAlivePeriod' noc='0' nocc='' rw='1' type='unsignedInt'></node>
		<node acl='' getc='0' name='NATDetected' noc='0' nocc='' rw='0' type='boolean'>false</node>
		<node acl='' getc='0' name='ManageableDeviceNumberOfEntries' noc='0' nocc='' rw='0' type='unsignedInt'></node>
		<node il='32' name='ManageableDevice' nin='1' rw='1' type='node'>
			<node name='template' rw='1' type='node'>
				<node acl='' getc='0' name='ManufacturerOUI' noc='0' nocc='' rw='0' type='string'></node>
				<node acl='' getc='0' name='SerialNumber' noc='0' nocc='' rw='0' type='string'></node>
				<node acl='' getc='0' name='ProductClass' noc='0' nocc='' rw='0' type='string'></node>
			</node>
		</node>
	</node>
	<node name='Time' rw='0' type='node'>
		<node acl='' getc='0' name='NTPServer1' noc='0' nocc='' rw='1' type='string' cmd='nvram' arg='ntp_server0' act='restart_time'></node>
		<node acl='' getc='0' name='CurrentLocalTime' noc='0' nocc='' rw='0' type='dateTime' cmd='currentlocaltime'></node>
		<node acl='' getc='0' name='LocalTimeZoneName' noc='0' nocc='' rw='1' type='string' cmd='localtimezonename' act='restart_time'></node>
	</node>
	<node name='UserInterface' rw='0' type='node'>
		<node acl='' getc='0' name='AvailableLanguages' noc='0' nocc='' rw='0' type='string' cmd='availablelanguages'></node>
		<node acl='' getc='0' name='CurrentLanguage' noc='0' nocc='' rw='1' type='string' cmd='nvram' arg='preferred_lang'></node>
	</node>
	<node name='Layer3Forwarding' rw='0' type='node'>
		<node acl='' getc='0' name='ForwardNumberOfEntries' noc='0' nocc='' rw='0' type='unsignedInt'></node>
		<node il='32' name='Forwarding' nin='1' rw='1' type='node' cmd='l3_forwarding' act='restart_net'>
			<node name='template' rw='1' type='node'>
				<node acl='' getc='0' name='StaticRoute' noc='0' nocc='' rw='0' type='boolean' act='restart_net'>true</node>
				<node acl='' getc='0' name='DestIPAddress' noc='0' nocc='' rw='1' type='string' cmd='l3_destip' act='restart_net'></node>
				<node acl='' getc='0' name='DestSubnetMask' noc='0' nocc='' rw='1' type='string' cmd='l3_destnetmask' act='restart_net'></node>
				<node acl='' getc='0' name='GatewayIPAddress' noc='0' nocc='' rw='1' type='string' cmd='l3_gatewayip' act='restart_net'></node>
				<node acl='' getc='0' name='Interface' noc='0' nocc='' rw='1' type='string' cmd='l3_iface' act='restart_net'></node>
				<node acl='' getc='0' name='ForwardingMetric' noc='0' nocc='' rw='1' type='int' cmd='l3_metric' act='restart_net'>-1</node>
			</node>
		</node>
	</node>
	<node name='IPPingDiagnostics' rw='0' type='node'>
		<node acl='' getc='0' name='DiagnosticsState' noc='0' nocc='' rw='1' type='string'></node>
		<node acl='' getc='0' name='Interface' noc='0' nocc='' rw='1' type='string'></node>
		<node acl='' getc='0' name='Host' noc='0' nocc='' rw='1' type='string'></node>
		<node acl='' getc='0' name='NumberOfRepetitions' noc='0' nocc='' rw='1' type='unsignedInt'></node>
		<node acl='' getc='0' name='Timeout' noc='0' nocc='' rw='1' type='unsignedInt'></node>
		<node acl='' getc='0' name='DataBlockSize' noc='0' nocc='' rw='1' type='unsignedInt'></node>
		<node acl='' getc='0' name='DSCP' noc='0' nocc='' rw='1' type='unsignedInt'></node>
		<node acl='' getc='0' name='SuccessCount' noc='0' nocc='' rw='0' type='unsignedInt'></node>
		<node acl='' getc='0' name='FailureCount' noc='0' nocc='' rw='0' type='unsignedInt'></node>
		<node acl='' getc='0' name='AverageResponseTime' noc='0' nocc='' rw='0' type='unsignedInt'></node>
		<node acl='' getc='0' name='MinimumResponseTime' noc='0' nocc='' rw='0' type='unsignedInt'></node>
		<node acl='' getc='0' name='MaximumResponseTime' noc='0' nocc='' rw='0' type='unsignedInt'></node>
	</node>
	<node name='TraceRouteDiagnostics' rw='0' type='node'>
		<node acl='' getc='0' name='DiagnosticsState' noc='0' nocc='' rw='1' type='string'></node>
		<node acl='' getc='0' name='Interface' noc='0' nocc='' rw='1' type='string'></node>
		<node acl='' getc='0' name='Host' noc='0' nocc='' rw='1' type='string'></node>
		<node acl='' getc='0' name='NumberOfTries' noc='0' nocc='' rw='1' type='unsignedInt'></node>
		<node acl='' getc='0' name='Timeout' noc='0' nocc='' rw='1' type='unsignedInt'></node>
		<node acl='' getc='0' name='DataBlockSize' noc='0' nocc='' rw='1' type='unsignedInt'></node>
		<node acl='' getc='0' name='DSCP' noc='0' nocc='' rw='1' type='unsignedInt'></node>
		<node acl='' getc='0' name='MaxHopCount' noc='0' nocc='' rw='1' type='unsignedInt'></node>
		<node acl='' getc='0' name='ResponseTime' noc='0' nocc='' rw='0' type='unsignedInt'>0</node>
		<node acl='' getc='0' name='RouteHopsNumberOfEntries' noc='0' nocc='' rw='0' type='unsignedInt'></node>
		<node il='64' name='RouteHops' nin='1' rw='1' type='node'>
			<node name='template' rw='1' type='node'>
				<node acl='' getc='0' name='HopHost' noc='0' nocc='' rw='0' type='string'></node>
				<node acl='' getc='0' name='HopHostAddress' noc='0' nocc='' rw='0' type='string'></node>
				<node acl='' getc='0' name='HopRTTimes' noc='0' nocc='' rw='0' type='string'></node>
			</node>
		</node>
	</node>
	<node name='DownloadDiagnostics' rw='0' type='node'>
		<node acl='' getc='0' name='DiagnosticsState' noc='0' nocc='' rw='1' type='string'></node>
		<node acl='' getc='0' name='Interface' noc='0' nocc='' rw='1' type='string'></node>
		<node acl='' getc='0' name='DownloadURL' noc='0' nocc='' rw='1' type='string'></node>
		<node acl='' getc='0' name='DSCP' noc='0' nocc='' rw='1' type='unsignedInt'>0</node>
		<node acl='' getc='0' name='EthernetPriority' noc='0' nocc='' rw='1' type='unsignedInt'>0</node>
		<node acl='' getc='0' name='ROMTime' noc='0' nocc='' rw='0' type='dateTime'></node>
		<node acl='' getc='0' name='BOMTime' noc='0' nocc='' rw='0' type='dateTime'></node>
		<node acl='' getc='0' name='EOMTime' noc='0' nocc='' rw='0' type='dateTime'></node>
		<node acl='' getc='0' name='TestBytesReceived' noc='0' nocc='' rw='0' type='unsignedInt'></node>
		<node acl='' getc='0' name='TotalBytesReceived' noc='0' nocc='' rw='0' type='unsignedInt'></node>
		<node acl='' getc='0' name='TCPOpenRequestTime' noc='0' nocc='' rw='0' type='dateTime'></node>
		<node acl='' getc='0' name='TCPOpenResponseTime' noc='0' nocc='' rw='0' type='dateTime'></node>
	</node>
	<node name='UploadDiagnostics' rw='0' type='node'>
		<node acl='' getc='0' name='DiagnosticsState' noc='0' nocc='' rw='1' type='string'></node>
		<node acl='' getc='0' name='Interface' noc='0' nocc='' rw='1' type='string'></node>
		<node acl='' getc='0' name='UploadURL' noc='0' nocc='' rw='1' type='string'></node>
		<node acl='' getc='0' name='DSCP' noc='0' nocc='' rw='1' type='unsignedInt'></node>
		<node acl='' getc='0' name='EthernetPriority' noc='0' nocc='' rw='1' type='unsignedInt'></node>
		<node acl='' getc='0' name='TestFileLength' noc='0' nocc='' rw='1' type='unsignedInt'></node>
		<node acl='' getc='0' name='ROMTime' noc='0' nocc='' rw='0' type='dateTime'></node>
		<node acl='' getc='0' name='BOMTime' noc='0' nocc='' rw='0' type='dateTime'></node>
		<node acl='' getc='0' name='EOMTime' noc='0' nocc='' rw='0' type='dateTime'></node>
		<node acl='' getc='0' name='TotalBytesSent' noc='0' nocc='' rw='0' type='unsignedInt'></node>
		<node acl='' getc='0' name='TCPOpenRequestTime' noc='0' nocc='' rw='0' type='dateTime'></node>
		<node acl='' getc='0' name='TCPOpenResponseTime' noc='0' nocc='' rw='0' type='dateTime'></node>
	</node>
	<node name='UDPEchoConfig' rw='0' type='node'>
		<node acl='' getc='0' name='Enable' noc='0' nocc='' rw='1' type='boolean'></node>
		<node acl='' getc='0' name='Interface' noc='0' nocc='' rw='1' type='string'></node>
		<node acl='' getc='0' name='SourceIPAddress' noc='0' nocc='' rw='1' type='string'></node>
		<node acl='' getc='0' name='UDPPort' noc='0' nocc='' rw='1' type='unsignedInt'></node>
		<node acl='' getc='0' name='EchoPlusEnabled' noc='0' nocc='' rw='1' type='boolean'></node>
		<node acl='' getc='0' name='EchoPlusSupported' noc='0' nocc='' rw='0' type='boolean'></node>
		<node acl='' getc='0' name='PacketsReceived' noc='0' nocc='' rw='0' type='unsignedInt'></node>
		<node acl='' getc='0' name='PacketsResponded' noc='0' nocc='' rw='0' type='unsignedInt'></node>
		<node acl='' getc='0' name='BytesReceived' noc='0' nocc='' rw='0' type='unsignedInt'></node>
		<node acl='' getc='0' name='BytesResponded' noc='0' nocc='' rw='0' type='unsignedInt'></node>
		<node acl='' getc='0' name='TimeFirstPacketReceived' noc='0' nocc='' rw='0' type='dateTime'></node>
		<node acl='' getc='0' name='TimeLastPacketReceived' noc='0' nocc='' rw='0' type='dateTime'></node>
	</node>
	<node name='LANDevice' rw='0' type='node' cmd='deny_object'>
		<node name='1' rw='0' type='node'>
			<node acl='' getc='0' name='LANEthernetInterfaceNumberOfEntries' noc='0' nocc='' rw='0' type='unsignedInt'>0</node>
			<node acl='' getc='0' name='LANWLANConfigurationNumberOfEntries' noc='0' nocc='' rw='0' type='unsignedInt'>0</node>
			<node name='LANHostConfigManagement' rw='0' type='node'>
				<node acl='' getc='0' name='DHCPServerEnable' noc='0' nocc='' rw='1' type='boolean' cmd='nvram' arg='dhcp_enable_x' act='restart_net_and_phy'></node>
				<node acl='' getc='0' name='MinAddress' noc='0' nocc='' rw='1' type='string' cmd='nvram' arg='dhcp_start' act='restart_net_and_phy'></node>
				<node acl='' getc='0' name='MaxAddress' noc='0' nocc='' rw='1' type='string' cmd='nvram' arg='dhcp_end' act='restart_net_and_phy'></node>
				<node acl='' getc='0' name='DomainName' noc='0' nocc='' rw='1' type='string' cmd='nvram' arg='lan_domain' act='restart_net_and_phy'></node>
				<node acl='' getc='0' name='DNSServers' noc='0' nocc='' rw='1' type='string' cmd='lanhost_dnsservers' act='restart_net_and_phy'></node>
				<node acl='' getc='0' name='IPRouters' noc='0' nocc='' rw='1' type='string' cmd='nvram' arg='dhcp_gateway_x' act='restart_net_and_phy'></node>
				<node acl='' getc='0' name='DHCPLeaseTime' noc='0' nocc='' rw='1' type='int' cmd='nvram' arg='dhcp_lease' act='restart_net_and_phy'></node>
				<node acl='' getc='0' name='AllowedMACAddresses' noc='0' nocc='' rw='1' type='string' cmd='allowedmacaddress' act='restart_wireless'></node>
				<node acl='' getc='0' name='IPInterfaceNumberOfEntries' noc='0' nocc='' rw='0' type='unsignedInt'>1</node>
				<node acl='' getc='0' name='DHCPStaticAddressNumberOfEntries' noc='0' nocc='' rw='0' type='unsignedInt'>0</node>
				<node name='IPInterface' rw='0' type='node'>
					<node name='1' rw='0' type='node'>
						<node acl='' getc='0' name='IPInterfaceIPAddress' noc='0' nocc='' rw='1' type='string' cmd='nvram' arg='lan_ipaddr' act='restart_net_and_phy'></node>
						<node acl='' getc='0' name='IPInterfaceSubnetMask' noc='0' nocc='' rw='1' type='string' cmd='nvram' arg='lan_netmask' act='restart_net_and_phy'></node>
					</node>
				</node>
				<node il='32' name='DHCPStaticAddress' nin='1' rw='1' type='node' cmd='lanhost_dhcpstatic' act='restart_net_and_phy'>
					<node name='template' rw='1' type='node'>
						<node acl='' getc='0' name='Chaddr' noc='0' nocc='' rw='1' type='string' cmd='lanhost_mac' act='restart_net_and_phy'></node>
						<node acl='' getc='0' name='Yiaddr' noc='0' nocc='' rw='1' type='string' cmd='lanhost_ip' act='restart_net_and_phy'></node>
					</node>
				</node>
			</node>
			<node il='9' name='WLANConfiguration' nin='1' rw='1' type='node' cmd='deny_object'>
				<node name='1' rw='0' type='node'>
					<node acl='' getc='0' name='Name' noc='0' nocc='' rw='0' type='string' cmd='lan_wlan_name'></node>
					<node acl='' getc='0' name='BSSID' noc='0' nocc='' rw='0' type='string' cmd='lan_wlan_bssid'></node>
					<node acl='' getc='0' name='Channel' noc='0' nocc='' rw='1' type='unsignedInt' cmd='lan_wlan_channel' act='restart_wireless'></node>
					<node acl='' getc='0' name='AutoChannelEnable' noc='0' nocc='' rw='1' type='boolean' cmd='lan_wlan_autochannel_enable' act='restart_wireless'></node>
					<node acl='' getc='0' name='SSID' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_ssid' act='restart_wireless'></node>
					<node acl='' getc='0' name='BeaconType' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_beacontype' act='restart_wireless'></node>
					<node acl='' getc='0' name='MACAddressControlEnabled' noc='0' nocc='' rw='1' type='boolean' cmd='lan_wlan_macaddrcontrolenable' act='restart_wireless'></node>
					<node acl='' getc='0' name='WEPKeyIndex' noc='0' nocc='' rw='1' type='unsignedInt' cmd='lan_wlan_wepkeyindex' act='restart_wireless'></node>
					<node acl='' getc='0' name='KeyPassphrase' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_keypassphrase' act='restart_wireless'></node>
					<node acl='' getc='0' name='WEPEncryptionLevel' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wepencryptionlevel'></node>
					<node acl='' getc='0' name='BasicEncryptionModes' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_basicencrytionmodes' act='restart_wireless'></node>
					<node acl='' getc='0' name='BasicAuthenticationMode' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_basicauthenticationmodes' act='restart_wireless'></node>
					<node acl='' getc='0' name='WPAEncryptionModes' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wpaencryptionmodes' act='restart_wireless'></node>
					<node acl='' getc='0' name='WPAAuthenticationMode' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wpaauthenticationmode' act='restart_wireless'></node>
					<node acl='' getc='0' name='IEEE11iEncryptionModes' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_ieee11iencryptionmodes' act='restart_wireless'></node>
					<node acl='' getc='0' name='IEEE11iAuthenticationMode' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_ieee11iauthenticationmode' act='restart_wireless'></node>
					<node acl='' getc='0' name='SSIDAdvertisementEnabled' noc='0' nocc='' rw='1' type='boolean' cmd='lan_wlan_ssid_endable' act='restart_wireless'></node>
					<node acl='' getc='0' name='RadioEnabled' noc='0' nocc='' rw='1' type='boolean' cmd='lan_wlan_radioenable' act='restart_wireless'></node>
					<node acl='' getc='0' name='TotalBytesSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_totalbytessent'></node>
					<node acl='' getc='0' name='TotalBytesReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_totalbytesreceived'></node>
					<node acl='' getc='0' name='TotalPacketsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_totalpacketssent'></node>
					<node acl='' getc='0' name='TotalPacketsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_totalpacketsreceived'></node>
					<node acl='' getc='0' name='TotalAssociations' noc='0' nocc='' rw='1' type='unsignedInt' cmd='lan_wlan_totalassociations'></node>
					<node name='Stats' rw='0' type='node'>
						<node acl='' getc='0' name='ErrorsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_errorssent'></node>
						<node acl='' getc='0' name='ErrorsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_errorsreceived'></node>
						<node acl='' getc='0' name='DiscardPacketsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_discardpacketssent'></node>
						<node acl='' getc='0' name='DiscardPacketsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_discardpacketsreceived'></node>
					</node>
					<node name='WPS' rw='0' type='node'>
						<node acl='' getc='0' name='Enable' noc='0' nocc='' rw='1' type='boolean' cmd='lan_wlan_wpsenable' act='restart_wireless'></node>
						<node acl='' getc='0' name='DevicePassword' noc='0' nocc='' rw='1' type='unsignedInt' cmd='lan_wlan_wpsdevicepassword' act='restart_wireless'></node>
						<node acl='' getc='0' name='ConfigMethodsEnabled' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wpsconfigmethodsenabled' act='start_wps_method'></node>
						<node acl='' getc='0' name='ConfigurationState' noc='0' nocc='' rw='0' type='string' cmd='lan_wlan_wpsconfigurationstate'></node>
					</node>
					<node il='9' name='AssociatedDevice' nin='1' rw='1' type='node'>
						<node name='template' rw='1' type='node'>
							<node acl='' getc='0' name='AssociatedDeviceMACAddress' noc='0' nocc='' rw='0' type='string'>00:00:00:00:00:00</node>
						</node>
					</node>
					<node name='WEPKey' rw='0' type='node'>
						<node name='1' rw='0' type='node'>
							<node acl='' getc='0' name='WEPKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wep_key' act='restart_wireless'></node>
						</node>
						<node name='2' rw='0' type='node'>
							<node acl='' getc='0' name='WEPKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wep_key' act='restart_wireless'></node>
						</node>
						<node name='3' rw='0' type='node'>
							<node acl='' getc='0' name='WEPKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wep_key' act='restart_wireless'></node>
						</node>
						<node name='4' rw='0' type='node'>
							<node acl='' getc='0' name='WEPKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wep_key' act='restart_wireless'></node>
						</node>
					</node>
					<node name='PreSharedKey' rw='0' type='node'>
						<node name='1' rw='0' type='node'>
							<node acl='' getc='0' name='PreSharedKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_presharedkey' act='restart_wireless'></node>
						</node>
					</node>
				</node>
				<node name='2' rw='1' type='node'>
					<node acl='' getc='0' name='Name' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_name'></node>
					<node acl='' getc='0' name='BSSID' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_bssid'></node>
					<node acl='' getc='0' name='Channel' noc='0' nocc='' rw='1' type='unsignedInt' cmd='lan_wlan_channel' act='restart_wireless'></node>
					<node acl='' getc='0' name='AutoChannelEnable' noc='0' nocc='' rw='1' type='boolean' cmd='lan_wlan_autochannel_enable' act='restart_wireless'></node>
					<node acl='' getc='0' name='SSID' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_ssid' act='restart_wireless'></node>
					<node acl='' getc='0' name='BeaconType' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_beacontype' act='restart_wireless'></node>
					<node acl='' getc='0' name='MACAddressControlEnabled' noc='0' nocc='' rw='1' type='boolean' cmd='lan_wlan_macaddrcontrolenable' act='restart_wireless'></node>
					<node acl='' getc='0' name='WEPKeyIndex' noc='0' nocc='' rw='1' type='unsignedInt' cmd='lan_wlan_wepkeyindex' act='restart_wireless'></node>
					<node acl='' getc='0' name='KeyPassphrase' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_keypassphrase' act='restart_wireless'></node>
					<node acl='' getc='0' name='WEPEncryptionLevel' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wepencryptionlevel'></node>
					<node acl='' getc='0' name='BasicEncryptionModes' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_basicencrytionmodes' act='restart_wireless'></node>
					<node acl='' getc='0' name='BasicAuthenticationMode' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_basicauthenticationmodes' act='restart_wireless'></node>
					<node acl='' getc='0' name='WPAEncryptionModes' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wpaencryptionmodes' act='restart_wireless'></node>
					<node acl='' getc='0' name='WPAAuthenticationMode' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wpaauthenticationmode' act='restart_wireless'></node>
					<node acl='' getc='0' name='IEEE11iEncryptionModes' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_ieee11iencryptionmodes' act='restart_wireless'></node>
					<node acl='' getc='0' name='IEEE11iAuthenticationMode' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_ieee11iauthenticationmode' act='restart_wireless'></node>
					<node acl='' getc='0' name='SSIDAdvertisementEnabled' noc='0' nocc='' rw='1' type='boolean' cmd='lan_wlan_ssid_endable' act='restart_wireless'></node>
					<node acl='' getc='0' name='RadioEnabled' noc='0' nocc='' rw='1' type='boolean' cmd='lan_wlan_radioenable' act='restart_wireless'></node>
					<node acl='' getc='0' name='TotalBytesSent' noc='0' nocc='' rw='1' type='unsignedInt' cmd='lan_wlan_totalbytessent'></node>
					<node acl='' getc='0' name='TotalBytesReceived' noc='0' nocc='' rw='1' type='unsignedInt' cmd='lan_wlan_totalbytesreceived'></node>
					<node acl='' getc='0' name='TotalPacketsSent' noc='0' nocc='' rw='1' type='unsignedInt' cmd='lan_wlan_totalpacketssent'></node>
					<node acl='' getc='0' name='TotalPacketsReceived' noc='0' nocc='' rw='1' type='unsignedInt' cmd='lan_wlan_totalpacketsreceived'></node>
					<node acl='' getc='0' name='TotalAssociations' noc='0' nocc='' rw='1' type='unsignedInt' cmd='lan_wlan_totalassociations'></node>
					<node name='Stats' rw='1' type='node'>
						<node acl='' getc='0' name='ErrorsSent' noc='0' nocc='' rw='1' type='unsignedInt' cmd='lan_wlan_errorssent'></node>
						<node acl='' getc='0' name='ErrorsReceived' noc='0' nocc='' rw='1' type='unsignedInt' cmd='lan_wlan_errorsreceived'></node>
						<node acl='' getc='0' name='DiscardPacketsSent' noc='0' nocc='' rw='1' type='unsignedInt' cmd='lan_wlan_discardpacketssent'></node>
						<node acl='' getc='0' name='DiscardPacketsReceived' noc='0' nocc='' rw='1' type='unsignedInt' cmd='lan_wlan_discardpacketsreceived'></node>
					</node>
					<node name='WPS' rw='1' type='node'>
						<node acl='' getc='0' name='Enable' noc='0' nocc='' rw='1' type='boolean' cmd='lan_wlan_wpsenable' act='restart_wireless'></node>
						<node acl='' getc='0' name='DevicePassword' noc='0' nocc='' rw='1' type='unsignedInt' cmd='lan_wlan_wpsdevicepassword' act='restart_wireless'></node>
						<node acl='' getc='0' name='ConfigMethodsEnabled' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wpsconfigmethodsenabled' act='start_wps_method'></node>
						<node acl='' getc='0' name='ConfigurationState' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wpsconfigurationstate'></node>
					</node>
					<node il='9' name='AssociatedDevice' nin='1' rw='1' type='node'>
						<node name='template' rw='1' type='node'>
							<node acl='' getc='0' name='AssociatedDeviceMACAddress' noc='0' nocc='' rw='1' type='string'>00:00:00:00:00:00</node>
						</node>
					</node>
					<node name='WEPKey' rw='1' type='node'>
						<node name='1' rw='1' type='node'>
							<node acl='' getc='0' name='WEPKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wep_key' act='restart_wireless'></node>
						</node>
						<node name='2' rw='1' type='node'>
							<node acl='' getc='0' name='WEPKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wep_key' act='restart_wireless'></node>
						</node>
						<node name='3' rw='1' type='node'>
							<node acl='' getc='0' name='WEPKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wep_key' act='restart_wireless'></node>
						</node>
						<node name='4' rw='1' type='node'>
							<node acl='' getc='0' name='WEPKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wep_key' act='restart_wireless'></node>
						</node>
					</node>
					<node name='PreSharedKey' rw='1' type='node'>
						<node name='1' rw='1' type='node'>
							<node acl='' getc='0' name='PreSharedKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_presharedkey' act='restart_wireless'></node>
						</node>
					</node>
				</node>
				<node name='3' rw='1' type='node'>
					<node acl='' getc='0' name='BSSID' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_bssid'></node>
					<node acl='' getc='0' name='SSID' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_ssid' act='restart_wireless'></node>
					<node acl='' getc='0' name='BeaconType' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_beacontype' act='restart_wireless'></node>
					<node acl='' getc='0' name='RadioEnabled' noc='0' nocc='' rw='1' type='boolean' cmd='lan_wlan_radioenable' act='restart_wireless'></node>
					<node acl='' getc='0' name='MACAddressControlEnabled' noc='0' nocc='' rw='1' type='boolean' cmd='lan_wlan_macaddrcontrolenable' act='restart_wireless'></node>
					<node acl='' getc='0' name='WEPKeyIndex' noc='0' nocc='' rw='1' type='unsignedInt' cmd='lan_wlan_wepkeyindex' act='restart_wireless'></node>
					<node acl='' getc='0' name='KeyPassphrase' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_keypassphrase' act='restart_wireless'></node>
					<node acl='' getc='0' name='WEPEncryptionLevel' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wepencryptionlevel'></node>
					<node acl='' getc='0' name='BasicEncryptionModes' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_basicencrytionmodes' act='restart_wireless'></node>
					<node acl='' getc='0' name='BasicAuthenticationMode' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_basicauthenticationmodes' act='restart_wireless'></node>
					<node acl='' getc='0' name='WPAEncryptionModes' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wpaencryptionmodes' act='restart_wireless'></node>
					<node acl='' getc='0' name='WPAAuthenticationMode' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wpaauthenticationmode' act='restart_wireless'></node>
					<node acl='' getc='0' name='IEEE11iEncryptionModes' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_ieee11iencryptionmodes' act='restart_wireless'></node>
					<node acl='' getc='0' name='IEEE11iAuthenticationMode' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_ieee11iauthenticationmode' act='restart_wireless'></node>
					<node acl='' getc='0' name='TotalBytesSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_totalbytessent'></node>
					<node acl='' getc='0' name='TotalBytesReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_totalbytesreceived'></node>
					<node acl='' getc='0' name='TotalPacketsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_totalpacketssent'></node>
					<node acl='' getc='0' name='TotalPacketsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_totalpacketsreceived'></node>
					<node acl='' getc='0' name='TotalAssociations' noc='0' nocc='' rw='1' type='unsignedInt' cmd='lan_wlan_totalassociations'></node>
					<node name='Stats' rw='0' type='node'>
						<node acl='' getc='0' name='ErrorsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_errorssent'></node>
						<node acl='' getc='0' name='ErrorsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_errorsreceived'></node>
						<node acl='' getc='0' name='DiscardPacketsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_discardpacketssent'></node>
						<node acl='' getc='0' name='DiscardPacketsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_discardpacketsreceived'></node>
					</node>
					<node il='9' name='AssociatedDevice' nin='1' rw='1' type='node'>
						<node name='template' rw='1' type='node'>
							<node acl='' getc='0' name='AssociatedDeviceMACAddress' noc='0' nocc='' rw='0' type='string'>00:00:00:00:00:00</node>
						</node>
					</node>
					<node name='WEPKey' rw='1' type='node'>
						<node name='1' rw='1' type='node'>
							<node acl='' getc='0' name='WEPKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wep_key' act='restart_wireless'></node>
						</node>
						<node name='2' rw='1' type='node'>
							<node acl='' getc='0' name='WEPKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wep_key' act='restart_wireless'></node>
						</node>
						<node name='3' rw='1' type='node'>
							<node acl='' getc='0' name='WEPKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wep_key' act='restart_wireless'></node>
						</node>
						<node name='4' rw='1' type='node'>
							<node acl='' getc='0' name='WEPKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wep_key' act='restart_wireless'></node>
						</node>
					</node>
					<node name='PreSharedKey' rw='0' type='node'>
						<node name='1' rw='0' type='node'>
							<node acl='' getc='0' name='PreSharedKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_presharedkey' act='restart_wireless'></node>
						</node>
					</node>
				</node>
				<node name='4' rw='1' type='node'>
					<node acl='' getc='0' name='BSSID' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_bssid'></node>
					<node acl='' getc='0' name='SSID' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_ssid' act='restart_wireless'></node>
					<node acl='' getc='0' name='BeaconType' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_beacontype' act='restart_wireless'></node>
					<node acl='' getc='0' name='RadioEnabled' noc='0' nocc='' rw='1' type='boolean' cmd='lan_wlan_radioenable' act='restart_wireless'></node>
					<node acl='' getc='0' name='MACAddressControlEnabled' noc='0' nocc='' rw='1' type='boolean' cmd='lan_wlan_macaddrcontrolenable' act='restart_wireless'></node>
					<node acl='' getc='0' name='WEPKeyIndex' noc='0' nocc='' rw='1' type='unsignedInt' cmd='lan_wlan_wepkeyindex' act='restart_wireless'></node>
					<node acl='' getc='0' name='KeyPassphrase' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_keypassphrase' act='restart_wireless'></node>
					<node acl='' getc='0' name='WEPEncryptionLevel' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wepencryptionlevel'></node>
					<node acl='' getc='0' name='BasicEncryptionModes' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_basicencrytionmodes' act='restart_wireless'></node>
					<node acl='' getc='0' name='BasicAuthenticationMode' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_basicauthenticationmodes' act='restart_wireless'></node>
					<node acl='' getc='0' name='WPAEncryptionModes' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wpaencryptionmodes' act='restart_wireless'></node>
					<node acl='' getc='0' name='WPAAuthenticationMode' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wpaauthenticationmode' act='restart_wireless'></node>
					<node acl='' getc='0' name='IEEE11iEncryptionModes' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_ieee11iencryptionmodes' act='restart_wireless'></node>
					<node acl='' getc='0' name='IEEE11iAuthenticationMode' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_ieee11iauthenticationmode' act='restart_wireless'></node>
					<node acl='' getc='0' name='TotalBytesSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_totalbytessent'></node>
					<node acl='' getc='0' name='TotalBytesReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_totalbytesreceived'></node>
					<node acl='' getc='0' name='TotalPacketsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_totalpacketssent'></node>
					<node acl='' getc='0' name='TotalPacketsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_totalpacketsreceived'></node>
					<node acl='' getc='0' name='TotalAssociations' noc='0' nocc='' rw='1' type='unsignedInt' cmd='lan_wlan_totalassociations'></node>
					<node name='Stats' rw='0' type='node'>
						<node acl='' getc='0' name='ErrorsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_errorssent'></node>
						<node acl='' getc='0' name='ErrorsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_errorsreceived'></node>
						<node acl='' getc='0' name='DiscardPacketsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_discardpacketssent'></node>
						<node acl='' getc='0' name='DiscardPacketsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_discardpacketsreceived'></node>
					</node>
					<node il='9' name='AssociatedDevice' nin='1' rw='1' type='node'>
						<node name='template' rw='1' type='node'>
							<node acl='' getc='0' name='AssociatedDeviceMACAddress' noc='0' nocc='' rw='0' type='string'>00:00:00:00:00:00</node>
						</node>
					</node>
					<node name='WEPKey' rw='1' type='node'>
						<node name='1' rw='1' type='node'>
							<node acl='' getc='0' name='WEPKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wep_key' act='restart_wireless'></node>
						</node>
						<node name='2' rw='1' type='node'>
							<node acl='' getc='0' name='WEPKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wep_key' act='restart_wireless'></node>
						</node>
						<node name='3' rw='1' type='node'>
							<node acl='' getc='0' name='WEPKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wep_key' act='restart_wireless'></node>
						</node>
						<node name='4' rw='1' type='node'>
							<node acl='' getc='0' name='WEPKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wep_key' act='restart_wireless'></node>
						</node>
					</node>
					<node name='PreSharedKey' rw='0' type='node'>
						<node name='1' rw='0' type='node'>
							<node acl='' getc='0' name='PreSharedKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_presharedkey' act='restart_wireless'></node>
						</node>
					</node>
				</node>
				<node name='5' rw='1' type='node'>
					<node acl='' getc='0' name='BSSID' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_bssid'></node>
					<node acl='' getc='0' name='SSID' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_ssid' act='restart_wireless'></node>
					<node acl='' getc='0' name='BeaconType' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_beacontype' act='restart_wireless'></node>
					<node acl='' getc='0' name='RadioEnabled' noc='0' nocc='' rw='1' type='boolean' cmd='lan_wlan_radioenable' act='restart_wireless'></node>
					<node acl='' getc='0' name='MACAddressControlEnabled' noc='0' nocc='' rw='1' type='boolean' cmd='lan_wlan_macaddrcontrolenable' act='restart_wireless'></node>
					<node acl='' getc='0' name='WEPKeyIndex' noc='0' nocc='' rw='1' type='unsignedInt' cmd='lan_wlan_wepkeyindex' act='restart_wireless'></node>
					<node acl='' getc='0' name='KeyPassphrase' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_keypassphrase' act='restart_wireless'></node>
					<node acl='' getc='0' name='WEPEncryptionLevel' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wepencryptionlevel'></node>
					<node acl='' getc='0' name='BasicEncryptionModes' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_basicencrytionmodes' act='restart_wireless'></node>
					<node acl='' getc='0' name='BasicAuthenticationMode' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_basicauthenticationmodes' act='restart_wireless'></node>
					<node acl='' getc='0' name='WPAEncryptionModes' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wpaencryptionmodes' act='restart_wireless'></node>
					<node acl='' getc='0' name='WPAAuthenticationMode' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wpaauthenticationmode' act='restart_wireless'></node>
					<node acl='' getc='0' name='IEEE11iEncryptionModes' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_ieee11iencryptionmodes' act='restart_wireless'></node>
					<node acl='' getc='0' name='IEEE11iAuthenticationMode' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_ieee11iauthenticationmode' act='restart_wireless'></node>
					<node acl='' getc='0' name='TotalBytesSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_totalbytessent'></node>
					<node acl='' getc='0' name='TotalBytesReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_totalbytesreceived'></node>
					<node acl='' getc='0' name='TotalPacketsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_totalpacketssent'></node>
					<node acl='' getc='0' name='TotalPacketsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_totalpacketsreceived'></node>
					<node acl='' getc='0' name='TotalAssociations' noc='0' nocc='' rw='1' type='unsignedInt' cmd='lan_wlan_totalassociations'></node>
					<node name='Stats' rw='0' type='node'>
						<node acl='' getc='0' name='ErrorsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_errorssent'></node>
						<node acl='' getc='0' name='ErrorsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_errorsreceived'></node>
						<node acl='' getc='0' name='DiscardPacketsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_discardpacketssent'></node>
						<node acl='' getc='0' name='DiscardPacketsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_discardpacketsreceived'></node>
					</node>
					<node il='9' name='AssociatedDevice' nin='1' rw='1' type='node'>
						<node name='template' rw='1' type='node'>
							<node acl='' getc='0' name='AssociatedDeviceMACAddress' noc='0' nocc='' rw='0' type='string'>00:00:00:00:00:00</node>
						</node>
					</node>
					<node name='WEPKey' rw='1' type='node'>
						<node name='1' rw='1' type='node'>
							<node acl='' getc='0' name='WEPKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wep_key' act='restart_wireless'></node>
						</node>
						<node name='2' rw='1' type='node'>
							<node acl='' getc='0' name='WEPKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wep_key' act='restart_wireless'></node>
						</node>
						<node name='3' rw='1' type='node'>
							<node acl='' getc='0' name='WEPKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wep_key' act='restart_wireless'></node>
						</node>
						<node name='4' rw='1' type='node'>
							<node acl='' getc='0' name='WEPKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wep_key' act='restart_wireless'></node>
						</node>
					</node>
					<node name='PreSharedKey' rw='0' type='node'>
						<node name='1' rw='0' type='node'>
							<node acl='' getc='0' name='PreSharedKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_presharedkey' act='restart_wireless'></node>
						</node>
					</node>
				</node>
				<node name='6' rw='1' type='node'>
					<node acl='' getc='0' name='BSSID' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_bssid'></node>
					<node acl='' getc='0' name='SSID' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_ssid' act='restart_wireless'></node>
					<node acl='' getc='0' name='BeaconType' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_beacontype' act='restart_wireless'></node>
					<node acl='' getc='0' name='RadioEnabled' noc='0' nocc='' rw='1' type='boolean' cmd='lan_wlan_radioenable' act='restart_wireless'></node>
					<node acl='' getc='0' name='MACAddressControlEnabled' noc='0' nocc='' rw='1' type='boolean' cmd='lan_wlan_macaddrcontrolenable' act='restart_wireless'></node>
					<node acl='' getc='0' name='WEPKeyIndex' noc='0' nocc='' rw='1' type='unsignedInt' cmd='lan_wlan_wepkeyindex' act='restart_wireless'></node>
					<node acl='' getc='0' name='KeyPassphrase' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_keypassphrase' act='restart_wireless'></node>
					<node acl='' getc='0' name='WEPEncryptionLevel' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wepencryptionlevel'></node>
					<node acl='' getc='0' name='BasicEncryptionModes' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_basicencrytionmodes' act='restart_wireless'></node>
					<node acl='' getc='0' name='BasicAuthenticationMode' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_basicauthenticationmodes' act='restart_wireless'></node>
					<node acl='' getc='0' name='WPAEncryptionModes' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wpaencryptionmodes' act='restart_wireless'></node>
					<node acl='' getc='0' name='WPAAuthenticationMode' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wpaauthenticationmode' act='restart_wireless'></node>
					<node acl='' getc='0' name='IEEE11iEncryptionModes' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_ieee11iencryptionmodes' act='restart_wireless'></node>
					<node acl='' getc='0' name='IEEE11iAuthenticationMode' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_ieee11iauthenticationmode' act='restart_wireless'></node>
					<node acl='' getc='0' name='TotalBytesSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_totalbytessent'></node>
					<node acl='' getc='0' name='TotalBytesReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_totalbytesreceived'></node>
					<node acl='' getc='0' name='TotalPacketsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_totalpacketssent'></node>
					<node acl='' getc='0' name='TotalPacketsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_totalpacketsreceived'></node>
					<node acl='' getc='0' name='TotalAssociations' noc='0' nocc='' rw='1' type='unsignedInt' cmd='lan_wlan_totalassociations'></node>
					<node name='Stats' rw='0' type='node'>
						<node acl='' getc='0' name='ErrorsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_errorssent'></node>
						<node acl='' getc='0' name='ErrorsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_errorsreceived'></node>
						<node acl='' getc='0' name='DiscardPacketsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_discardpacketssent'></node>
						<node acl='' getc='0' name='DiscardPacketsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_discardpacketsreceived'></node>
					</node>
					<node il='9' name='AssociatedDevice' nin='1' rw='1' type='node'>
						<node name='template' rw='1' type='node'>
							<node acl='' getc='0' name='AssociatedDeviceMACAddress' noc='0' nocc='' rw='0' type='string'>00:00:00:00:00:00</node>
						</node>
					</node>
					<node name='WEPKey' rw='1' type='node'>
						<node name='1' rw='1' type='node'>
							<node acl='' getc='0' name='WEPKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wep_key' act='restart_wireless'></node>
						</node>
						<node name='2' rw='1' type='node'>
							<node acl='' getc='0' name='WEPKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wep_key' act='restart_wireless'></node>
						</node>
						<node name='3' rw='1' type='node'>
							<node acl='' getc='0' name='WEPKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wep_key' act='restart_wireless'></node>
						</node>
						<node name='4' rw='1' type='node'>
							<node acl='' getc='0' name='WEPKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wep_key' act='restart_wireless'></node>
						</node>
					</node>
					<node name='PreSharedKey' rw='0' type='node'>
						<node name='1' rw='0' type='node'>
							<node acl='' getc='0' name='PreSharedKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_presharedkey' act='restart_wireless'></node>
						</node>
					</node>
				</node>
				<node name='7' rw='1' type='node'>
					<node acl='' getc='0' name='BSSID' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_bssid'></node>
					<node acl='' getc='0' name='SSID' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_ssid' act='restart_wireless'></node>
					<node acl='' getc='0' name='BeaconType' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_beacontype' act='restart_wireless'></node>
					<node acl='' getc='0' name='RadioEnabled' noc='0' nocc='' rw='1' type='boolean' cmd='lan_wlan_radioenable' act='restart_wireless'></node>
					<node acl='' getc='0' name='MACAddressControlEnabled' noc='0' nocc='' rw='1' type='boolean' cmd='lan_wlan_macaddrcontrolenable' act='restart_wireless'></node>
					<node acl='' getc='0' name='WEPKeyIndex' noc='0' nocc='' rw='1' type='unsignedInt' cmd='lan_wlan_wepkeyindex' act='restart_wireless'></node>
					<node acl='' getc='0' name='KeyPassphrase' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_keypassphrase' act='restart_wireless'></node>
					<node acl='' getc='0' name='WEPEncryptionLevel' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wepencryptionlevel'></node>
					<node acl='' getc='0' name='BasicEncryptionModes' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_basicencrytionmodes' act='restart_wireless'></node>
					<node acl='' getc='0' name='BasicAuthenticationMode' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_basicauthenticationmodes' act='restart_wireless'></node>
					<node acl='' getc='0' name='WPAEncryptionModes' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wpaencryptionmodes' act='restart_wireless'></node>
					<node acl='' getc='0' name='WPAAuthenticationMode' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wpaauthenticationmode' act='restart_wireless'></node>
					<node acl='' getc='0' name='IEEE11iEncryptionModes' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_ieee11iencryptionmodes' act='restart_wireless'></node>
					<node acl='' getc='0' name='IEEE11iAuthenticationMode' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_ieee11iauthenticationmode' act='restart_wireless'></node>
					<node acl='' getc='0' name='TotalBytesSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_totalbytessent'></node>
					<node acl='' getc='0' name='TotalBytesReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_totalbytesreceived'></node>
					<node acl='' getc='0' name='TotalPacketsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_totalpacketssent'></node>
					<node acl='' getc='0' name='TotalPacketsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_totalpacketsreceived'></node>
					<node acl='' getc='0' name='TotalAssociations' noc='0' nocc='' rw='1' type='unsignedInt' cmd='lan_wlan_totalassociations'></node>
					<node name='Stats' rw='0' type='node'>
						<node acl='' getc='0' name='ErrorsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_errorssent'></node>
						<node acl='' getc='0' name='ErrorsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_errorsreceived'></node>
						<node acl='' getc='0' name='DiscardPacketsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_discardpacketssent'></node>
						<node acl='' getc='0' name='DiscardPacketsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_discardpacketsreceived'></node>
					</node>
					<node il='9' name='AssociatedDevice' nin='1' rw='1' type='node'>
						<node name='template' rw='1' type='node'>
							<node acl='' getc='0' name='AssociatedDeviceMACAddress' noc='0' nocc='' rw='0' type='string'>00:00:00:00:00:00</node>
						</node>
					</node>
					<node name='WEPKey' rw='1' type='node'>
						<node name='1' rw='1' type='node'>
							<node acl='' getc='0' name='WEPKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wep_key' act='restart_wireless'></node>
						</node>
						<node name='2' rw='1' type='node'>
							<node acl='' getc='0' name='WEPKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wep_key' act='restart_wireless'></node>
						</node>
						<node name='3' rw='1' type='node'>
							<node acl='' getc='0' name='WEPKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wep_key' act='restart_wireless'></node>
						</node>
						<node name='4' rw='1' type='node'>
							<node acl='' getc='0' name='WEPKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wep_key' act='restart_wireless'></node>
						</node>
					</node>
					<node name='PreSharedKey' rw='0' type='node'>
						<node name='1' rw='0' type='node'>
							<node acl='' getc='0' name='PreSharedKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_presharedkey' act='restart_wireless'></node>
						</node>
					</node>
				</node>
				<node name='8' rw='1' type='node'>
					<node acl='' getc='0' name='BSSID' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_bssid'></node>
					<node acl='' getc='0' name='SSID' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_ssid' act='restart_wireless'></node>
					<node acl='' getc='0' name='BeaconType' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_beacontype' act='restart_wireless'></node>
					<node acl='' getc='0' name='RadioEnabled' noc='0' nocc='' rw='1' type='boolean' cmd='lan_wlan_radioenable' act='restart_wireless'></node>
					<node acl='' getc='0' name='MACAddressControlEnabled' noc='0' nocc='' rw='1' type='boolean' cmd='lan_wlan_macaddrcontrolenable' act='restart_wireless'></node>
					<node acl='' getc='0' name='WEPKeyIndex' noc='0' nocc='' rw='1' type='unsignedInt' cmd='lan_wlan_wepkeyindex' act='restart_wireless'></node>
					<node acl='' getc='0' name='KeyPassphrase' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_keypassphrase' act='restart_wireless'></node>
					<node acl='' getc='0' name='WEPEncryptionLevel' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wepencryptionlevel'></node>
					<node acl='' getc='0' name='BasicEncryptionModes' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_basicencrytionmodes' act='restart_wireless'></node>
					<node acl='' getc='0' name='BasicAuthenticationMode' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_basicauthenticationmodes' act='restart_wireless'></node>
					<node acl='' getc='0' name='WPAEncryptionModes' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wpaencryptionmodes' act='restart_wireless'></node>
					<node acl='' getc='0' name='WPAAuthenticationMode' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wpaauthenticationmode' act='restart_wireless'></node>
					<node acl='' getc='0' name='IEEE11iEncryptionModes' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_ieee11iencryptionmodes' act='restart_wireless'></node>
					<node acl='' getc='0' name='IEEE11iAuthenticationMode' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_ieee11iauthenticationmode' act='restart_wireless'></node>
					<node acl='' getc='0' name='TotalBytesSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_totalbytessent'></node>
					<node acl='' getc='0' name='TotalBytesReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_totalbytesreceived'></node>
					<node acl='' getc='0' name='TotalPacketsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_totalpacketssent'></node>
					<node acl='' getc='0' name='TotalPacketsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_totalpacketsreceived'></node>
					<node acl='' getc='0' name='TotalAssociations' noc='0' nocc='' rw='1' type='unsignedInt' cmd='lan_wlan_totalassociations'></node>
					<node name='Stats' rw='0' type='node'>
						<node acl='' getc='0' name='ErrorsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_errorssent'></node>
						<node acl='' getc='0' name='ErrorsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_errorsreceived'></node>
						<node acl='' getc='0' name='DiscardPacketsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_discardpacketssent'></node>
						<node acl='' getc='0' name='DiscardPacketsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='lan_wlan_discardpacketsreceived'></node>
					</node>
					<node il='9' name='AssociatedDevice' nin='1' rw='1' type='node'>
						<node name='template' rw='1' type='node'>
							<node acl='' getc='0' name='AssociatedDeviceMACAddress' noc='0' nocc='' rw='0' type='string'>00:00:00:00:00:00</node>
						</node>
					</node>
					<node name='WEPKey' rw='1' type='node'>
						<node name='1' rw='1' type='node'>
							<node acl='' getc='0' name='WEPKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wep_key' act='restart_wireless'></node>
						</node>
						<node name='2' rw='1' type='node'>
							<node acl='' getc='0' name='WEPKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wep_key' act='restart_wireless'></node>
						</node>
						<node name='3' rw='1' type='node'>
							<node acl='' getc='0' name='WEPKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wep_key' act='restart_wireless'></node>
						</node>
						<node name='4' rw='1' type='node'>
							<node acl='' getc='0' name='WEPKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_wep_key' act='restart_wireless'></node>
						</node>
					</node>
					<node name='PreSharedKey' rw='0' type='node'>
						<node name='1' rw='0' type='node'>
							<node acl='' getc='0' name='PreSharedKey' noc='0' nocc='' rw='1' type='string' cmd='lan_wlan_presharedkey' act='restart_wireless'></node>
						</node>
					</node>
				</node>
            </node>
		</node>
	</node>
	<node name='WANDevice' rw='0' type='node'>
		<node name='1' rw='0' type='node'>
			<node acl='' getc='0' name='WANConnectionNumberOfEntries' noc='0' nocc='' rw='0' type='unsignedInt'>0</node>
			<node name='WANCommonInterfaceConfig' rw='0' type='node'>
				<node acl='' getc='0' name='WANAccessType' noc='0' nocc='' rw='0' type='string'>DSL</node>
				<node acl='' getc='0' name='PhysicalLinkStatus' noc='0' nocc='' rw='0' type='string' cmd='eth_linkstatus'></node>
			</node>
			<node name='WANDSLInterfaceConfig' rw='0' type='node'>
				<node acl='' getc='0' name='Status' noc='0' nocc='' rw='0' type='string' cmd='dsl_if_config' arg='lineState'></node>
				<node acl='' getc='0' name='ModulationType' noc='0' nocc='' rw='0' type='string' cmd='dsl_if_config' arg='Opmode'></node>
				<node acl='' getc='0' name='InterleaveDepth' noc='0' nocc='' rw='0' type='unsignedInt' cmd='dsl_if_config' arg='InterleaveDepth'></node>
				<node acl='' getc='0' name='UpstreamCurrRate' noc='0' nocc='' rw='0' type='unsignedInt' cmd='dsl_if_config' arg='DataRateUp'></node>
				<node acl='' getc='0' name='DownstreamCurrRate' noc='0' nocc='' rw='0' type='unsignedInt' cmd='dsl_if_config' arg='DataRateDown'></node>
				<node acl='' getc='0' name='UpstreamMaxRate' noc='0' nocc='' rw='0' type='unsignedInt' cmd='dsl_if_config' arg='AttainUp'></node>
				<node acl='' getc='0' name='DownstreamMaxRate' noc='0' nocc='' rw='0' type='unsignedInt' cmd='dsl_if_config' arg='AttainDown'></node>
				<node acl='' getc='0' name='UpstreamNoiseMargin' noc='0' nocc='' rw='0' type='int' cmd='dsl_if_config' arg='SNRMarginUp'></node>
				<node acl='' getc='0' name='DownstreamNoiseMargin' noc='0' nocc='' rw='0' type='int' cmd='dsl_if_config' arg='SNRMarginDown'></node>
				<node acl='' getc='0' name='UpstreamAttenuation' noc='0' nocc='' rw='0' type='int' cmd='dsl_if_config' arg='AttenUp'></node>
				<node acl='' getc='0' name='DownstreamAttenuation' noc='0' nocc='' rw='0' type='int' cmd='dsl_if_config' arg='AttenDown'></node>
				<node acl='' getc='0' name='UpstreamPower' noc='0' nocc='' rw='0' type='int' cmd='dsl_if_config' arg='PowerUp'></node>
				<node acl='' getc='0' name='DownstreamPower' noc='0' nocc='' rw='0' type='int' cmd='dsl_if_config' arg='PowerDown'></node>
				<node acl='' getc='0' name='ATURANSIStd' noc='0' nocc='' rw='0' type='unsignedInt' cmd='dsl_if_config' arg='ATURANSIStd'></node>
				<node acl='' getc='0' name='ATURANSIRev' noc='0' nocc='' rw='0' type='unsignedInt' cmd='dsl_if_config' arg='ATURANSIRev'></node>
				<node acl='' getc='0' name='ATUCANSIStd' noc='0' nocc='' rw='0' type='unsignedInt' cmd='dsl_if_config' arg='ATUCANSIStd'></node>
				<node acl='' getc='0' name='ATUCANSIRev' noc='0' nocc='' rw='0' type='unsignedInt' cmd='dsl_if_config' arg='ATUCANSIRev'></node>
				<node acl='' getc='0' name='TotalStart' noc='0' nocc='' rw='0' type='unsignedInt' cmd='dsl_if_config' arg='TotalStart'></node>
				<node acl='' getc='0' name='ShowtimeStart' noc='0' nocc='' rw='0' type='unsignedInt' cmd='dsl_if_config' arg='ShowtimeStart'>></node>
			</node>		
			<node il='16' name='WANConnectionDevice' nin='1' rw='1' type='node' cmd='dsl_wan_connection_device' act='restart_dslwan_if'>
				<node name='template' rw='1' type='node'>
					<node acl='' getc='0' name='WANIPConnectionNumberOfEntries' noc='0' nocc='' rw='0' type='unsignedInt'>0</node>
					<node acl='' getc='0' name='WANPPPConnectionNumberOfEntries' noc='0' nocc='' rw='0' type='unsignedInt'>0</node>
					<node name='WANDSLLinkConfig' rw='0' type='node'>
						<node acl='' getc='0' name='Enable' noc='0' nocc='' rw='1' type='boolean' cmd='dsl_enable' act='restart_dslwan_if'>false</node>
						<node acl='' getc='0' name='LinkStatus' noc='0' nocc='' rw='0' type='string' cmd='nvram' arg='dsltmp_adslsyncsts'></node>
						<node acl='' getc='0' name='LinkType' noc='0' nocc='' rw='0' type='string' cmd='dsl_link_type'>Unconfigured</node>
						<node acl='' getc='0' name='ModulationType' noc='0' nocc='' rw='0' type='string' cmd='nvram' arg='dsllog_opmode'></node>
						<node acl='' getc='0' name='DestinationAddress' noc='0' nocc='' rw='1' type='string' cmd='dsl_dest_addr' act='restart_dslwan_if'></node>
						<node acl='' getc='0' name='ATMEncapsulation' noc='0' nocc='' rw='1' type='string' cmd='dsl_atm_encap' act='restart_dslwan_if'></node>
						<node acl='' getc='0' name='ATMQoS' noc='0' nocc='' rw='1' type='string' cmd='dsl_atm_qos' act='reboot'></node>
						<node acl='' getc='0' name='ATMPeakCellRate' noc='0' nocc='' rw='1' type='unsignedInt' cmd='dsl_atm_pcr' act='restart_dslwan_if'></node>
						<node acl='' getc='0' name='ATMMaximumBurstSize' noc='0' nocc='' rw='1' type='unsignedInt' cmd='dsl_atm_mbs' act='restart_dslwan_if'></node>
						<node acl='' getc='0' name='ATMSustainableCellRate' noc='0' nocc='' rw='1' type='unsignedInt' cmd='dsl_atm_scr' act='restart_dslwan_if'></node>
						<node acl='' getc='0' name='X_ASUS_EnableDot1q' noc='0' nocc='' rw='1' type='boolean' cmd='dsl_enable_dot1q' act='restart_dslwan_if'></node>
						<node acl='' getc='0' name='X_ASUS_Dot1qVid' noc='0' nocc='' rw='1' type='unsignedInt' cmd='dsl_dot1q_vid' act='restart_dslwan_if'></node>
						<node acl='' getc='0' name='X_ASUS_DSLIndex' noc='0' nocc='' rw='0' type='string'></node>
					</node>
					<node il='1' name='WANIPConnection' nin='1' rw='1' type='node' cmd='dsl_wanip_connection' act='restart_dslwan_if'>
						<node name='template' rw='1' type='node'>
							<node acl='' getc='0' name='Enable' noc='0' nocc='' rw='1' type='boolean' cmd='eth_wanip_enable' act='restart_dslwan_if'>false</node>
							<node acl='' getc='0' name='ConnectionStatus' noc='0' nocc='' rw='0' type='string' cmd='eth_wanip_connectionstatus'></node>
							<node acl='' getc='0' name='NATEnabled' noc='0' nocc='' rw='1' type='boolean' cmd='eth_wanip_natenabled' act='restart_dslwan_if'></node>
							<node acl='' getc='0' name='AddressingType' noc='0' nocc='' rw='1' type='string' cmd='eth_wanip_addressingtype' act='restart_dslwan_if'></node>
							<node acl='' getc='0' name='ExternalIPAddress' noc='0' nocc='' rw='1' type='string' cmd='eth_wanip_externalipaddress' act='restart_dslwan_if'></node>
							<node acl='' getc='0' name='SubnetMask' noc='0' nocc='' rw='1' type='string' cmd='eth_wanip_subnetmask' act='restart_dslwan_if'></node>
							<node acl='' getc='0' name='DefaultGateway' noc='0' nocc='' rw='1' type='string' cmd='eth_wanip_defaultgateway' act='restart_dslwan_if'></node>
							<node acl='' getc='0' name='DNSOverrideAllowed' noc='0' nocc='' rw='1' type='boolean' cmd='eth_wanip_dnsoverrideallowed' act='restart_dslwan_if'>false</node>
							<node acl='' getc='0' name='DNSServers' noc='0' nocc='' rw='1' type='string' cmd='eth_wanip_dnsservers' act='restart_dslwan_if'></node>
							<node acl='' getc='0' name='ConnectionType' noc='0' nocc='' rw='1' type='string' cmd='eth_wanip_connectiontype' act='restart_wan_if'></node>
							<node acl='' getc='0' name='PortMappingNumberOfEntries' noc='0' nocc='' rw='0' type='unsignedInt'>0</node>
							<node il='32' name='PortMapping' nin='1' rw='1' type='node' cmd='eth_portmapping' act='restart_firewall'>
								<node name='template' rw='1' type='node'>
									<node acl='' getc='0' name='ExternalPort' noc='0' nocc='' rw='1' type='unsignedInt' cmd='eth_externalport' act='restart_firewall'></node>
									<node acl='' getc='0' name='ExternalPortEndRange' noc='0' nocc='' rw='1' type='unsignedInt' cmd='eth_externalportendrange' act='restart_firewall'>0</node>
									<node acl='' getc='0' name='InternalPort' noc='0' nocc='' rw='1' type='unsignedInt' cmd='eth_internalport' act='restart_firewall'></node>
									<node acl='' getc='0' name='PortMappingProtocol' noc='0' nocc='' rw='1' type='string' cmd='eth_portmappingprotocol' act='restart_firewall'></node>
									<node acl='' getc='0' name='InternalClient' noc='0' nocc='' rw='1' type='string' cmd='eth_internalclient' act='restart_firewall'></node>
									<node acl='' getc='0' name='RemoteHost' noc='0' nocc='' rw='1' type='string' cmd='eth_remotehost' act='restart_firewall'></node>
									<node acl='' getc='0' name='PortMappingDescription' noc='0' nocc='' rw='1' type='string' cmd='eth_portmappingdescription' act='restart_firewall'></node>
								</node>
							</node>
							<node name='Stats' rw='0' type='node'>
								<node acl='' getc='0' name='EthernetBytesSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanip_bytessent'></node>
								<node acl='' getc='0' name='EthernetBytesReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanip_bytesreceived'></node>
								<node acl='' getc='0' name='EthernetPacketsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanip_packetssent'></node>
								<node acl='' getc='0' name='EthernetPacketsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanip_packetsreceived'></node>
								<node acl='' getc='0' name='EthernetErrorsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanip_errorssent'></node>
								<node acl='' getc='0' name='EthernetErrorsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanip_errorsreceived'></node>
								<node acl='' getc='0' name='EthernetDiscardPacketsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanip_discardpacketssent'></node>
								<node acl='' getc='0' name='EthernetDiscardPacketsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanip_discardpacketsreceived'></node>
							</node>
						</node>
					</node>
					<node il='1' name='WANPPPConnection' nin='1' rw='1' type='node' cmd='dsl_wanppp_connection' act='restart_dslwan_if'>
						<node name='template' rw='1' type='node'>
							<node acl='' getc='0' name='Enable' noc='0' nocc='' rw='1' type='boolean' cmd='eth_wanppp_enable' act='restart_dslwan_if'>false</node>
							<node acl='' getc='0' name='ConnectionStatus' noc='0' nocc='' rw='0' type='string' cmd='eth_wanppp_connectionstatus'></node>
							<node acl='' getc='0' name='DefaultGateway' noc='0' nocc='' rw='0' type='string' cmd='eth_wanppp_defaultgateway'></node>
							<node acl='' getc='0' name='IdleDisconnectTime' noc='0' nocc='' rw='1' type='unsignedInt' cmd='eth_wanppp_idledisconnecttime' act='restart_dslwan_if'></node>
							<node acl='' getc='0' name='NATEnabled' noc='0' nocc='' rw='1' type='boolean' cmd='eth_wanppp_natenabled' act='restart_dslwan_if'></node>
							<node acl='' getc='0' name='Username' noc='0' nocc='' rw='1' type='string' cmd='eth_wanppp_username' act='restart_dslwan_if'></node>
							<node acl='' getc='1' name='Password' noc='0' nocc='' rw='1' type='string' cmd='eth_wanppp_password' act='restart_dslwan_if'></node>
							<node acl='' getc='0' name='ExternalIPAddress' noc='0' nocc='' rw='0' type='string' cmd='eth_wanppp_externalipaddress'></node>
							<node acl='' getc='0' name='DNSServers' noc='0' nocc='' rw='1' type='string' cmd='eth_wanppp_dnsservers' act='restart_dslwan_if'></node>
							<node acl='' getc='0' name='TransportType' noc='0' nocc='' rw='0' type='string' cmd='eth_wanppp_transporttype'></node>
							<node acl='' getc='0' name='ConnectionType' noc='0' nocc='' rw='1' type='string' cmd='eth_wanppp_connectiontype' act='restart_wan_if'></node>
							<node acl='' getc='0' name='PPPoEACName' noc='0' nocc='' rw='1' type='string' cmd='eth_wanppp_pppoeacname'  act='restart_dslwan_if'></node>
							<node acl='' getc='0' name='PPPoEServiceName' noc='0' nocc='' rw='1' type='string' cmd='eth_wanppp_pppoeservicename'  act='restart_dslwan_if'></node>
							<node acl='' getc='0' name='PortMappingNumberOfEntries' noc='0' nocc='' rw='0' type='unsignedInt'>0</node>
							<node il='32' name='PortMapping' nin='1' rw='1' type='node' cmd='eth_portmapping' act='restart_firewall'>
								<node name='template' rw='1' type='node'>
									<node acl='' getc='0' name='ExternalPort' noc='0' nocc='' rw='1' type='unsignedInt' cmd='eth_externalport' act='restart_firewall'></node>
									<node acl='' getc='0' name='ExternalPortEndRange' noc='0' nocc='' rw='1' type='unsignedInt' cmd='eth_externalportendrange' act='restart_firewall'>0</node>
									<node acl='' getc='0' name='InternalPort' noc='0' nocc='' rw='1' type='unsignedInt' cmd='eth_internalport' act='restart_firewall'></node>
									<node acl='' getc='0' name='PortMappingProtocol' noc='0' nocc='' rw='1' type='string' cmd='eth_portmappingprotocol' act='restart_firewall'></node>
									<node acl='' getc='0' name='InternalClient' noc='0' nocc='' rw='1' type='string' cmd='eth_internalclient' act='restart_firewall'></node>
									<node acl='' getc='0' name='RemoteHost' noc='0' nocc='' rw='1' type='string' cmd='eth_remotehost' act='restart_firewall'></node>
									<node acl='' getc='0' name='PortMappingDescription' noc='0' nocc='' rw='1' type='string' cmd='eth_portmappingdescription' act='restart_firewall'></node>
								</node>
							</node>
							<node name='Stats' rw='0' type='node'>
								<node acl='' getc='0' name='EthernetBytesSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanppp_bytessent'></node>
								<node acl='' getc='0' name='EthernetBytesReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanppp_bytesreceived'></node>
								<node acl='' getc='0' name='EthernetPacketsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanppp_packetssent'></node>
								<node acl='' getc='0' name='EthernetPacketsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanppp_packetsreceived'></node>
								<node acl='' getc='0' name='EthernetErrorsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanppp_errorssent'></node>
								<node acl='' getc='0' name='EthernetErrorsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanppp_errorsreceived'></node>
								<node acl='' getc='0' name='EthernetDiscardPacketsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanppp_discardpacketsSent'></node>
								<node acl='' getc='0' name='EthernetDiscardPacketsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanppp_discardpacketsreceived'></node>
							</node>
						</node>
					</node>
				</node>
			</node>
		</node>
		<node name='2' rw='0' type='node'>
			<node acl='' getc='0' name='WANConnectionNumberOfEntries' noc='0' nocc='' rw='0' type='unsignedInt'>1</node>
			<node name='WANCommonInterfaceConfig' rw='0' type='node'>
				<node acl='' getc='0' name='WANAccessType' noc='0' nocc='' rw='0' type='string'>Ethernet</node>
				<node acl='' getc='0' name='PhysicalLinkStatus' noc='0' nocc='' rw='0' type='string' cmd='eth_linkstatus'></node>
			</node>
			<node name='WANConnectionDevice' rw='0' type='node'>
				<node name='1' rw='0' type='node'>
					<node acl='' getc='0' name='WANIPConnectionNumberOfEntries' noc='0' nocc='' rw='0' type='unsignedInt'>0</node>
					<node acl='' getc='0' name='WANPPPConnectionNumberOfEntries' noc='0' nocc='' rw='0' type='unsignedInt'>0</node>
					<node name='WANEthernetLinkConfig' rw='0' type='node'>
						<node acl='' getc='0' name='EthernetLinkStatus' noc='0' nocc='' rw='0' type='string' cmd='eth_linkstatus'></node>
					</node>
					<node il='1' name='WANIPConnection' nin='1' rw='1' type='node' cmd='eth_wanip_connection' act='restart_wan_if'>
						<node name='template' rw='1' type='node'>
							<node acl='' getc='0' name='Enable' noc='0' nocc='' rw='1' type='boolean' cmd='eth_wanip_enable' act='restart_wan_if'>false</node>
							<node acl='' getc='0' name='ConnectionStatus' noc='0' nocc='' rw='0' type='string' cmd='eth_wanip_connectionstatus'></node>
							<node acl='' getc='0' name='NATEnabled' noc='0' nocc='' rw='1' type='boolean' cmd='eth_wanip_natenabled' act='restart_wan_if'></node>
							<node acl='' getc='0' name='AddressingType' noc='0' nocc='' rw='1' type='string' cmd='eth_wanip_addressingtype' act='restart_wan_if'></node>
							<node acl='' getc='0' name='ExternalIPAddress' noc='0' nocc='' rw='1' type='string' cmd='eth_wanip_externalipaddress' act='restart_wan_if'></node>
							<node acl='' getc='0' name='SubnetMask' noc='0' nocc='' rw='1' type='string' cmd='eth_wanip_subnetmask' act='restart_wan_if'></node>
							<node acl='' getc='0' name='DefaultGateway' noc='0' nocc='' rw='1' type='string' cmd='eth_wanip_defaultgateway' act='restart_wan_if'></node>
							<node acl='' getc='0' name='DNSOverrideAllowed' noc='0' nocc='' rw='1' type='boolean' cmd='eth_wanip_dnsoverrideallowed' act='restart_wan_if'>false</node>
							<node acl='' getc='0' name='DNSServers' noc='0' nocc='' rw='1' type='string' cmd='eth_wanip_dnsservers' act='restart_wan_if'></node>
							<node acl='' getc='0' name='ConnectionType' noc='0' nocc='' rw='1' type='string' cmd='eth_wanip_connectiontype' act='restart_wan_if'></node>
							<node acl='' getc='0' name='PortMappingNumberOfEntries' noc='0' nocc='' rw='0' type='unsignedInt'>0</node>
							<node il='32' name='PortMapping' nin='1' rw='1' type='node' cmd='eth_portmapping' act='restart_firewall'>
								<node name='template' rw='1' type='node'>
									<node acl='' getc='0' name='ExternalPort' noc='0' nocc='' rw='1' type='unsignedInt' cmd='eth_externalport' act='restart_firewall'></node>
									<node acl='' getc='0' name='ExternalPortEndRange' noc='0' nocc='' rw='1' type='unsignedInt' cmd='eth_externalportendrange' act='restart_firewall'>0</node>
									<node acl='' getc='0' name='InternalPort' noc='0' nocc='' rw='1' type='unsignedInt' cmd='eth_internalport' act='restart_firewall'></node>
									<node acl='' getc='0' name='PortMappingProtocol' noc='0' nocc='' rw='1' type='string' cmd='eth_portmappingprotocol' act='restart_firewall'></node>
									<node acl='' getc='0' name='InternalClient' noc='0' nocc='' rw='1' type='string' cmd='eth_internalclient' act='restart_firewall'></node>
									<node acl='' getc='0' name='RemoteHost' noc='0' nocc='' rw='1' type='string' cmd='eth_remotehost' act='restart_firewall'></node>
									<node acl='' getc='0' name='PortMappingDescription' noc='0' nocc='' rw='1' type='string' cmd='eth_portmappingdescription' act='restart_firewall'></node>
								</node>
							</node>
							<node name='Stats' rw='0' type='node'>
								<node acl='' getc='0' name='EthernetBytesSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanip_bytessent'></node>
								<node acl='' getc='0' name='EthernetBytesReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanip_bytesreceived'></node>
								<node acl='' getc='0' name='EthernetPacketsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanip_packetssent'></node>
								<node acl='' getc='0' name='EthernetPacketsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanip_packetsreceived'></node>
								<node acl='' getc='0' name='EthernetErrorsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanip_errorssent'></node>
								<node acl='' getc='0' name='EthernetErrorsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanip_errorsreceived'></node>
								<node acl='' getc='0' name='EthernetDiscardPacketsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanip_discardpacketssent'></node>
								<node acl='' getc='0' name='EthernetDiscardPacketsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanip_discardpacketsreceived'></node>
							</node>
						</node>
					</node>
					<node il='1' name='WANPPPConnection' nin='1' rw='1' type='node' cmd='eth_wanppp_connection' act='restart_wan_if'>
						<node name='template' rw='1' type='node'>
							<node acl='' getc='0' name='Enable' noc='0' nocc='' rw='1' type='boolean' cmd='eth_wanppp_enable' act='restart_wan_if'>false</node>
							<node acl='' getc='0' name='ConnectionStatus' noc='0' nocc='' rw='0' type='string' cmd='eth_wanppp_connectionstatus'></node>
							<node acl='' getc='0' name='DefaultGateway' noc='0' nocc='' rw='0' type='string' cmd='eth_wanppp_defaultgateway'></node>
							<node acl='' getc='0' name='IdleDisconnectTime' noc='0' nocc='' rw='1' type='unsignedInt' cmd='eth_wanppp_idledisconnecttime' act='restart_wan_if'></node>
							<node acl='' getc='0' name='NATEnabled' noc='0' nocc='' rw='1' type='boolean' cmd='eth_wanppp_natenabled' act='restart_wan_if'></node>
							<node acl='' getc='0' name='Username' noc='0' nocc='' rw='1' type='string' cmd='eth_wanppp_username' act='restart_wan_if'></node>
							<node acl='' getc='1' name='Password' noc='0' nocc='' rw='1' type='string' cmd='eth_wanppp_password' act='restart_wan_if'></node>
							<node acl='' getc='0' name='ExternalIPAddress' noc='0' nocc='' rw='0' type='string' cmd='eth_wanppp_externalipaddress'></node>
							<node acl='' getc='0' name='DNSServers' noc='0' nocc='' rw='1' type='string' cmd='eth_wanppp_dnsservers' act='restart_wan_if'></node>
							<node acl='' getc='0' name='TransportType' noc='0' nocc='' rw='0' type='string' cmd='eth_wanppp_transporttype'></node>
							<node acl='' getc='0' name='ConnectionType' noc='0' nocc='' rw='1' type='string' cmd='eth_wanppp_connectiontype' act='restart_wan_if'></node>
							<node acl='' getc='0' name='PPPoEACName' noc='0' nocc='' rw='1' type='string' cmd='eth_wanppp_pppoeacname'  act='restart_wan_if'></node>
							<node acl='' getc='0' name='PPPoEServiceName' noc='0' nocc='' rw='1' type='string' cmd='eth_wanppp_pppoeservicename'  act='restart_wan_if'></node>
							<node acl='' getc='0' name='PortMappingNumberOfEntries' noc='0' nocc='' rw='0' type='unsignedInt'>0</node>
							<node il='32' name='PortMapping' nin='1' rw='1' type='node' cmd='eth_portmapping' act='restart_firewall'>
								<node name='template' rw='1' type='node'>
									<node acl='' getc='0' name='ExternalPort' noc='0' nocc='' rw='1' type='unsignedInt' cmd='eth_externalport' act='restart_firewall'></node>
									<node acl='' getc='0' name='ExternalPortEndRange' noc='0' nocc='' rw='1' type='unsignedInt' cmd='eth_externalportendrange' act='restart_firewall'>0</node>
									<node acl='' getc='0' name='InternalPort' noc='0' nocc='' rw='1' type='unsignedInt' cmd='eth_internalport' act='restart_firewall'></node>
									<node acl='' getc='0' name='PortMappingProtocol' noc='0' nocc='' rw='1' type='string' cmd='eth_portmappingprotocol' act='restart_firewall'></node>
									<node acl='' getc='0' name='InternalClient' noc='0' nocc='' rw='1' type='string' cmd='eth_internalclient' act='restart_firewall'></node>
									<node acl='' getc='0' name='RemoteHost' noc='0' nocc='' rw='1' type='string' cmd='eth_remotehost' act='restart_firewall'></node>
									<node acl='' getc='0' name='PortMappingDescription' noc='0' nocc='' rw='1' type='string' cmd='eth_portmappingdescription' act='restart_firewall'></node>
								</node>
							</node>
							<node name='Stats' rw='0' type='node'>
								<node acl='' getc='0' name='EthernetBytesSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanppp_bytessent'></node>
								<node acl='' getc='0' name='EthernetBytesReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanppp_bytesreceived'></node>
								<node acl='' getc='0' name='EthernetPacketsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanppp_packetssent'></node>
								<node acl='' getc='0' name='EthernetPacketsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanppp_packetsreceived'></node>
								<node acl='' getc='0' name='EthernetErrorsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanppp_errorssent'></node>
								<node acl='' getc='0' name='EthernetErrorsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanppp_errorsreceived'></node>
								<node acl='' getc='0' name='EthernetDiscardPacketsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanppp_discardpacketsSent'></node>
								<node acl='' getc='0' name='EthernetDiscardPacketsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanppp_discardpacketsreceived'></node>
							</node>
						</node>
					</node>
				</node>
			</node>
		</node>
		<node name='3' rw='0' type='node'>
			<node acl='' getc='0' name='WANConnectionNumberOfEntries' noc='0' nocc='' rw='0' type='unsignedInt'>1</node>
			<node name='WANCommonInterfaceConfig' rw='0' type='node'>
				<node acl='' getc='0' name='WANAccessType' noc='0' nocc='' rw='0' type='string'>Ethernet</node>
				<node acl='' getc='0' name='PhysicalLinkStatus' noc='0' nocc='' rw='0' type='string' cmd='eth_linkstatus'></node>
			</node>
			<node name='WANConnectionDevice' rw='0' type='node'>
				<node name='1' rw='0' type='node'>
					<node acl='' getc='0' name='WANIPConnectionNumberOfEntries' noc='0' nocc='' rw='0' type='unsignedInt'>0</node>
					<node acl='' getc='0' name='WANPPPConnectionNumberOfEntries' noc='0' nocc='' rw='0' type='unsignedInt'>0</node>
					<node name='WANEthernetLinkConfig' rw='0' type='node'>
						<node acl='' getc='0' name='EthernetLinkStatus' noc='0' nocc='' rw='0' type='string' cmd='eth_linkstatus'></node>
					</node>
					<node il='1' name='WANIPConnection' nin='1' rw='1' type='node' cmd='eth_wanip_connection' act='restart_wan_if'>
						<node name='template' rw='1' type='node'>
							<node acl='' getc='0' name='Enable' noc='0' nocc='' rw='1' type='boolean' cmd='eth_wanip_enable' act='restart_wan_if'>false</node>
							<node acl='' getc='0' name='ConnectionStatus' noc='0' nocc='' rw='0' type='string' cmd='eth_wanip_connectionstatus'></node>
							<node acl='' getc='0' name='NATEnabled' noc='0' nocc='' rw='1' type='boolean' cmd='eth_wanip_natenabled' act='restart_wan_if'></node>
							<node acl='' getc='0' name='AddressingType' noc='0' nocc='' rw='1' type='string' cmd='eth_wanip_addressingtype' act='restart_wan_if'></node>
							<node acl='' getc='0' name='ExternalIPAddress' noc='0' nocc='' rw='1' type='string' cmd='eth_wanip_externalipaddress' act='restart_wan_if'></node>
							<node acl='' getc='0' name='SubnetMask' noc='0' nocc='' rw='1' type='string' cmd='eth_wanip_subnetmask' act='restart_wan_if'></node>
							<node acl='' getc='0' name='DefaultGateway' noc='0' nocc='' rw='1' type='string' cmd='eth_wanip_defaultgateway' act='restart_wan_if'></node>
							<node acl='' getc='0' name='DNSOverrideAllowed' noc='0' nocc='' rw='1' type='boolean' cmd='eth_wanip_dnsoverrideallowed' act='restart_wan_if'>false</node>
							<node acl='' getc='0' name='DNSServers' noc='0' nocc='' rw='1' type='string' cmd='eth_wanip_dnsservers' act='restart_wan_if'></node>
							<node acl='' getc='0' name='ConnectionType' noc='0' nocc='' rw='1' type='string' cmd='eth_wanip_connectiontype' act='restart_wan_if'></node>
							<node acl='' getc='0' name='PortMappingNumberOfEntries' noc='0' nocc='' rw='0' type='unsignedInt'>0</node>
							<node il='32' name='PortMapping' nin='1' rw='1' type='node' cmd='eth_portmapping' act='restart_firewall'>
								<node name='template' rw='1' type='node'>
									<node acl='' getc='0' name='ExternalPort' noc='0' nocc='' rw='1' type='unsignedInt' cmd='eth_externalport' act='restart_firewall'></node>
									<node acl='' getc='0' name='ExternalPortEndRange' noc='0' nocc='' rw='1' type='unsignedInt' cmd='eth_externalportendrange' act='restart_firewall'>0</node>
									<node acl='' getc='0' name='InternalPort' noc='0' nocc='' rw='1' type='unsignedInt' cmd='eth_internalport' act='restart_firewall'></node>
									<node acl='' getc='0' name='PortMappingProtocol' noc='0' nocc='' rw='1' type='string' cmd='eth_portmappingprotocol' act='restart_firewall'></node>
									<node acl='' getc='0' name='InternalClient' noc='0' nocc='' rw='1' type='string' cmd='eth_internalclient' act='restart_firewall'></node>
									<node acl='' getc='0' name='RemoteHost' noc='0' nocc='' rw='1' type='string' cmd='eth_remotehost' act='restart_firewall'></node>
									<node acl='' getc='0' name='PortMappingDescription' noc='0' nocc='' rw='1' type='string' cmd='eth_portmappingdescription' act='restart_firewall'></node>
								</node>
							</node>
							<node name='Stats' rw='0' type='node'>
								<node acl='' getc='0' name='EthernetBytesSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanip_bytessent'></node>
								<node acl='' getc='0' name='EthernetBytesReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanip_bytesreceived'></node>
								<node acl='' getc='0' name='EthernetPacketsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanip_packetssent'></node>
								<node acl='' getc='0' name='EthernetPacketsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanip_packetsreceived'></node>
								<node acl='' getc='0' name='EthernetErrorsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanip_errorssent'></node>
								<node acl='' getc='0' name='EthernetErrorsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanip_errorsreceived'></node>
								<node acl='' getc='0' name='EthernetDiscardPacketsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanip_discardpacketssent'></node>
								<node acl='' getc='0' name='EthernetDiscardPacketsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanip_discardpacketsreceived'></node>
							</node>
						</node>
					</node>
					<node il='1' name='WANPPPConnection' nin='1' rw='1' type='node' cmd='eth_wanppp_connection' act='restart_wan_if'>
						<node name='template' rw='1' type='node'>
							<node acl='' getc='0' name='Enable' noc='0' nocc='' rw='1' type='boolean' cmd='eth_wanppp_enable' act='restart_wan_if'>false</node>
							<node acl='' getc='0' name='ConnectionStatus' noc='0' nocc='' rw='0' type='string' cmd='eth_wanppp_connectionstatus'></node>
							<node acl='' getc='0' name='DefaultGateway' noc='0' nocc='' rw='0' type='string' cmd='eth_wanppp_defaultgateway'></node>
							<node acl='' getc='0' name='IdleDisconnectTime' noc='0' nocc='' rw='1' type='unsignedInt' cmd='eth_wanppp_idledisconnecttime' act='restart_wan_if'></node>
							<node acl='' getc='0' name='NATEnabled' noc='0' nocc='' rw='1' type='boolean' cmd='eth_wanppp_natenabled' act='restart_wan_if'></node>
							<node acl='' getc='0' name='Username' noc='0' nocc='' rw='1' type='string' cmd='eth_wanppp_username' act='restart_wan_if'></node>
							<node acl='' getc='1' name='Password' noc='0' nocc='' rw='1' type='string' cmd='eth_wanppp_password' act='restart_wan_if'></node>
							<node acl='' getc='0' name='ExternalIPAddress' noc='0' nocc='' rw='0' type='string' cmd='eth_wanppp_externalipaddress'></node>
							<node acl='' getc='0' name='DNSServers' noc='0' nocc='' rw='1' type='string' cmd='eth_wanppp_dnsservers' act='restart_wan_if'></node>
							<node acl='' getc='0' name='TransportType' noc='0' nocc='' rw='0' type='string' cmd='eth_wanppp_transporttype'></node>
							<node acl='' getc='0' name='ConnectionType' noc='0' nocc='' rw='1' type='string' cmd='eth_wanppp_connectiontype' act='restart_wan_if'></node>
							<node acl='' getc='0' name='PPPoEACName' noc='0' nocc='' rw='1' type='string' cmd='eth_wanppp_pppoeacname'  act='restart_wan_if'></node>
							<node acl='' getc='0' name='PPPoEServiceName' noc='0' nocc='' rw='1' type='string' cmd='eth_wanppp_pppoeservicename'  act='restart_wan_if'></node>
							<node acl='' getc='0' name='PortMappingNumberOfEntries' noc='0' nocc='' rw='0' type='unsignedInt'>0</node>
							<node il='32' name='PortMapping' nin='1' rw='1' type='node' cmd='eth_portmapping' act='restart_firewall'>
								<node name='template' rw='1' type='node'>
									<node acl='' getc='0' name='ExternalPort' noc='0' nocc='' rw='1' type='unsignedInt' cmd='eth_externalport' act='restart_firewall'></node>
									<node acl='' getc='0' name='ExternalPortEndRange' noc='0' nocc='' rw='1' type='unsignedInt' cmd='eth_externalportendrange' act='restart_firewall'>0</node>
									<node acl='' getc='0' name='InternalPort' noc='0' nocc='' rw='1' type='unsignedInt' cmd='eth_internalport' act='restart_firewall'></node>
									<node acl='' getc='0' name='PortMappingProtocol' noc='0' nocc='' rw='1' type='string' cmd='eth_portmappingprotocol' act='restart_firewall'></node>
									<node acl='' getc='0' name='InternalClient' noc='0' nocc='' rw='1' type='string' cmd='eth_internalclient' act='restart_firewall'></node>
									<node acl='' getc='0' name='RemoteHost' noc='0' nocc='' rw='1' type='string' cmd='eth_remotehost' act='restart_firewall'></node>
									<node acl='' getc='0' name='PortMappingDescription' noc='0' nocc='' rw='1' type='string' cmd='eth_portmappingdescription' act='restart_firewall'></node>
								</node>
							</node>
							<node name='Stats' rw='0' type='node'>
								<node acl='' getc='0' name='EthernetBytesSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanppp_bytessent'></node>
								<node acl='' getc='0' name='EthernetBytesReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanppp_bytesreceived'></node>
								<node acl='' getc='0' name='EthernetPacketsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanppp_packetssent'></node>
								<node acl='' getc='0' name='EthernetPacketsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanppp_packetsreceived'></node>
								<node acl='' getc='0' name='EthernetErrorsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanppp_errorssent'></node>
								<node acl='' getc='0' name='EthernetErrorsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanppp_errorsreceived'></node>
								<node acl='' getc='0' name='EthernetDiscardPacketsSent' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanppp_discardpacketsSent'></node>
								<node acl='' getc='0' name='EthernetDiscardPacketsReceived' noc='0' nocc='' rw='0' type='unsignedInt' cmd='eth_wanppp_discardpacketsreceived'></node>
							</node>
						</node>
					</node>
				</node>
			</node>
		</node>
	</node>
	<node name='X_ASUS_Specific' rw='0' type='node'>
		<node acl='' getc='0' name='X_ASUS_CPUUsage' noc='0' nocc='' rw='0' type='unsignedInt' cmd='cpuusage'></node>
		<node name='X_ASUS_MemoryStatus' rw='0' type='node'>
			<node acl='' getc='0' name='X_ASUS_Total' noc='0' nocc='' rw='0' type='unsignedInt' cmd='totalmemory'></node>
			<node acl='' getc='0' name='X_ASUS_Free' noc='0' nocc='' rw='0' type='unsignedInt' cmd='freememory'></node>
		</node>
		<node name='X_ASUS_Wan' rw='0' type='node'>
			<node name='X_ASUS_Internet' rw='0' type='node'>
				<node name='X_ASUS_DslType' rw='0' type='node'>
					<node acl='' getc='0' name='X_ASUS_TransMode' noc='0' nocc='' rw='1' type='string' cmd='nvram' arg='dslx_transmode' act='reboot'></node>
				</node>
				<node name='X_ASUS_LanType' rw='0' type='node'>
					<node acl='' getc='0' name='X_ASUS_Wan_ConnectionType' noc='0' nocc='' rw='1' type='string' cmd='wan_connectiontype' act='restart_wan_if'></node>
					<node acl='' getc='0' name='X_ASUS_Wan_DhcpEnable' noc='0' nocc='' rw='1' type='boolean' cmd='wan_dhcpenable' act='restart_wan_if'></node>
					<node acl='' getc='0' name='X_ASUS_Wan_Ip' noc='0' nocc='' rw='1' type='string' cmd='wan_ip' act='restart_wan_if'></node>
					<node acl='' getc='0' name='X_ASUS_Wan_Netmask' noc='0' nocc='' rw='1' type='string' cmd='wan_netmask' act='restart_wan_if'></node>
					<node acl='' getc='0' name='X_ASUS_Wan_Gateway' noc='0' nocc='' rw='1' type='string' cmd='wan_gateway' act='restart_wan_if'></node>
					<node acl='' getc='0' name='X_ASUS_Wan_DnsEnable' noc='0' nocc='' rw='1' type='boolean' cmd='wan_dnsenable' act='restart_wan_if'></node>
					<node acl='' getc='0' name='X_ASUS_Wan_Dns1' noc='0' nocc='' rw='1' type='string' cmd='wan_dns1' act='restart_wan_if'></node>
					<node acl='' getc='0' name='X_ASUS_Wan_Dns2' noc='0' nocc='' rw='1' type='string' cmd='wan_dns2' act='restart_wan_if'></node>
					<node acl='' getc='0' name='X_ASUS_Wan_Username' noc='0' nocc='' rw='1' type='string' cmd='wan_username' act='restart_wan_if'></node>
					<node acl='' getc='0' name='X_ASUS_Wan_Password' noc='0' nocc='' rw='1' type='string' cmd='wan_password' act='restart_wan_if'></node>
					<node acl='' getc='0' name='X_ASUS_Wan_VpnServer' noc='0' nocc='' rw='1' type='string' cmd='wan_vpnserver' act='restart_wan_if'></node>
					<node acl='' getc='0' name='X_ASUS_Wan_Hostname' noc='0' nocc='' rw='1' type='string' cmd='wan_hostname' act='restart_wan_if'></node>
				</node>
				<node name='X_ASUS_UsbType' rw='0' type='node'>
					<node acl='' getc='0' name='X_ASUS_UsbModem_Enable' noc='0' nocc='' rw='1' type='boolean' cmd='nvram' arg='modem_enable' act='reboot'></node>
					<node acl='' getc='0' name='X_ASUS_UsbModem_Country' noc='0' nocc='' rw='1' type='string' cmd='nvram' arg='modem_country' act='reboot'></node>
					<node acl='' getc='0' name='X_ASUS_UsbModem_Isp' noc='0' nocc='' rw='1' type='string' cmd='nvram' arg='modem_isp' act='reboot'></node>
					<node acl='' getc='0' name='X_ASUS_UsbModem_Mode' noc='0' nocc='' rw='1' type='unsignedInt' cmd='nvram' arg='modem_mode' act='reboot'></node>
					<node acl='' getc='0' name='X_ASUS_UsbModem_Apn' noc='0' nocc='' rw='1' type='string' cmd='nvram' arg='modem_apn' act='reboot'></node>
					<node acl='' getc='0' name='X_ASUS_UsbModem_DialNumber' noc='0' nocc='' rw='1' type='string' cmd='nvram' arg='modem_dialnum' act='reboot'></node>
					<node acl='' getc='0' name='X_ASUS_UsbModem_PinCode' noc='0' nocc='' rw='1' type='string' cmd='nvram' arg='modem_pincode' act='reboot'></node>
					<node acl='' getc='0' name='X_ASUS_UsbModem_UserName' noc='0' nocc='' rw='1' type='string' cmd='nvram' arg='modem_user' act='reboot'></node>
					<node acl='' getc='0' name='X_ASUS_UsbModem_Password' noc='0' nocc='' rw='1' type='string' cmd='nvram' arg='modem_pass' act='reboot'></node>
					<node acl='' getc='0' name='X_ASUS_UsbModem_Type' noc='0' nocc='' rw='1' type='string' cmd='nvram' arg='Dev3G' act='reboot'></node>
				</node>
			</node>
			<node name='X_ASUS_DualWan' rw='0' type='node'>
				<node acl='' getc='0' name='X_ASUS_PrimaryWan' noc='0' nocc='' rw='1' type='string' cmd='primarywan' act='reboot'></node>
				<node acl='' getc='0' name='X_ASUS_SecondaryWan' noc='0' nocc='' rw='1' type='string' cmd='secondarywan' act='reboot'></node>
				<node acl='' getc='0' name='X_ASUS_LanPort' noc='0' nocc='' rw='1' type='unsignedInt' cmd='nvram' arg='wans_lanport' act='reboot'></node>
				<node acl='' getc='0' name='X_ASUS_DualWanMode' noc='0' nocc='' rw='1' type='string' cmd='nvram' arg='wans_mode' act='reboot'></node>
				<node acl='' getc='0' name='X_ASUS_LoadBalance_Ratio' noc='0' nocc='' rw='1' type='string' cmd='nvram' arg='wans_lb_ratio' act='reboot'></node>
				<node acl='' getc='0' name='X_ASUS_Wandog_Interval' noc='0' nocc='' rw='1' type='unsignedInt' cmd='nvram' arg='wandog_interval' act='reboot'></node>
				<node acl='' getc='0' name='X_ASUS_Wandog_Delay' noc='0' nocc='' rw='1' type='unsignedInt' cmd='nvram' arg='wandog_delay' act='reboot'></node>
				<node acl='' getc='0' name='X_ASUS_Wandog_MaxFail' noc='0' nocc='' rw='1' type='unsignedInt' cmd='nvram' arg='wandog_maxfail' act='reboot'></node>
				<node acl='' getc='0' name='X_ASUS_Wandog_Failback_Count' noc='0' nocc='' rw='1' type='unsignedInt' cmd='nvram' arg='wandog_fb_count' act='reboot'></node>
				<node acl='' getc='0' name='X_ASUS_Wandog_Enable' noc='0' nocc='' rw='1' type='boolean' cmd='nvram' arg='wandog_enable' act='reboot'></node>
			</node>
		</node>
	</node>
	<node acl='' getc='0' name='X_ASUS_CompleteXML' noc='0' nocc='' rw='0' type='boolean'>true</node>
</node>
