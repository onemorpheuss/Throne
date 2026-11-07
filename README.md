<div align="center">

<img src="res/public/Throne.png" width="128" alt="Throne logo">

# Throne

**Cross-platform Qt desktop proxy client, powered by [sing-box](https://github.com/SagerNet/sing-box)**

[![Release](https://img.shields.io/github/v/release/throneproj/Throne?style=flat-square&label=release&color=2F80ED)](https://github.com/throneproj/Throne/releases/latest)
[![Downloads](https://img.shields.io/github/downloads/throneproj/Throne/total?style=flat-square&logo=github&label=downloads)](https://github.com/throneproj/Throne/releases)
[![Stars](https://img.shields.io/github/stars/throneproj/Throne?style=flat-square&logo=github&label=stars)](https://github.com/throneproj/Throne/stargazers)
[![License](https://img.shields.io/github/license/throneproj/Throne?style=flat-square&color=6E7781)](LICENSE)
[![Telegram](https://img.shields.io/badge/Telegram-@ThroneProj-26A5E4?style=flat-square&logo=telegram&logoColor=white)](https://t.me/ThroneProj)

</div>

Throne manages proxy profiles and subscriptions, routes traffic with rule-based routing profiles, and works either as a system proxy or as a Tun VPN. It also runs headless, as a service or in a container, controlled with [thronectl](https://github.com/throneproj/Throne-api): see [Headless Throne](https://throneproj.github.io/headless/).

<p align="center">
  <img width="820" alt="Throne main window" src="https://github.com/user-attachments/assets/af4a8e32-7e55-430c-9402-ec2d665cf71a" />
</p>

## Installation

Portable ZIPs and installers for every platform are on [GitHub Releases](https://github.com/throneproj/Throne/releases). The [installation guide](https://throneproj.github.io/get_started/installation/) covers the rest, including the Linux install script, the APT and RPM repositories, the AUR and the other package managers.

### macOS
Apple platforms have a very strict security policy and since Throne does not have a signed certificate, you will have to remove the quarantine using `xattr -d com.apple.quarantine /path/to/throne.app`. Move `Throne.app` to `/Applications` before the first launch — the built-in privilege escalation opens `Terminal` to make the core setuid-root, and that step can fail while the app is still inside `~/Downloads`.

The Homebrew cask (macOS 14.4 or newer, Apple Silicon and Intel) does both for you; update with `brew upgrade --cask throne`.
```bash
brew install --cask throneproj/tap/throne
```

## Supported protocols

- SOCKS
- HTTP(S)
- Shadowsocks
- Trojan
- VMess
- VLESS
- TUIC
- Hysteria
- Hysteria2
- AnyTLS
- Mieru
- Snell
- NaïveProxy
- Juicity
- TrustTunnel
- ShadowTLS
- Wireguard
- AmneziaWG
- MASQUE
- SSH
- DNSTT / VayDNS
- MasterDNS
- Xray VLESS (XHTTP/XDrive)
- OpenVPN/OpenConnect
- Custom Outbound (Both Sing-box and Xray)
- Custom Config (Both Sing-box and Xray)
- Chaining outbounds
- Extra Core

## Subscription Formats

Various formats are supported, including share links, various JSON representation of Sing-box configs, and v2rayN link format as well as limited support for Shadowsocks and Clash formats.

Deeplinks are also supported, read the [documentation](https://throneproj.github.io/advanced/deeplinks/) for more information.
