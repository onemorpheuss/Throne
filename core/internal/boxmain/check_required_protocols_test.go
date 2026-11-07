//go:build with_quic && with_wireguard

package boxmain

import (
	"fmt"
	"strings"
	"testing"
)

// These fixtures validate the complete core configuration path without dialing
// a server. They guard the protocols shipped in the Windows release against
// missing build tags, registry entries, and incompatible option changes.
func TestCheckRequiredProtocolConfigs(t *testing.T) {
	const uuid = "00000000-0000-4000-8000-000000000000"
	testKey := strings.Repeat("A", 43) + "="

	tests := []struct {
		name   string
		config string
	}{
		{
			name: "VMess",
			config: fmt.Sprintf(`{
				"outbounds": [{"type":"vmess","tag":"proxy","server":"127.0.0.1","server_port":443,"uuid":%q,"security":"auto"}]
			}`, uuid),
		},
		{
			name: "VLESS",
			config: fmt.Sprintf(`{
				"outbounds": [{"type":"vless","tag":"proxy","server":"127.0.0.1","server_port":443,"uuid":%q}]
			}`, uuid),
		},
		{
			name: "Hysteria2",
			config: `{
				"outbounds": [{"type":"hysteria2","tag":"proxy","server":"127.0.0.1","server_port":443,"password":"test-password","tls":{"enabled":true,"server_name":"example.com"}}]
			}`,
		},
		{
			name: "TrustTunnel",
			config: `{
				"outbounds": [{"type":"trusttunnel","tag":"proxy","server":"127.0.0.1","server_port":443,"username":"test-user","password":"test-password","quic":true,"quic_congestion_control":"bbr","tls":{"enabled":true,"server_name":"example.com"}}]
			}`,
		},
		{
			name: "AmneziaWG",
			config: fmt.Sprintf(`{
				"endpoints": [{
					"type":"wireguard","tag":"proxy","address":["10.0.0.2/32"],"private_key":%q,
					"peers":[{"address":"127.0.0.1","port":51820,"public_key":%q,"allowed_ips":["0.0.0.0/0","::/0"]}],
					"amnezia_wg":{"jc":5,"jmin":10,"jmax":50,"s1":1,"s2":2,"s3":3,"s4":4,"h1":"1","h2":"2","h3":"3","h4":"4"}
				}],
				"outbounds": [{"type":"direct","tag":"direct"}]
			}`, testKey, testKey),
		},
	}

	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			if err := Check([]byte(test.config)); err != nil {
				t.Fatalf("valid %s config was rejected: %v", test.name, err)
			}
		})
	}
}
