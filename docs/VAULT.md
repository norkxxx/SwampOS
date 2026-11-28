# SwampVault & SwampLogin

## Implemented offline prototype (host only)
`tools/vault.py` uses Python cryptography (not a kernel dependency) with RSA-4096 OAEP/SHA-256 to wrap a fresh AES-256 key, random 96-bit nonce and AES-GCM to encrypt and authenticate a complete image (up to 16 MiB). Its fixed header is authenticated as AAD. RSA private key is password-encrypted PKCS#8 PEM using library defaults; this is not Argon2id or TPM sealing. No private keys are packaged. It checks authentication before creating an exclusive mode-0600 output. Python cannot guarantee secret zeroization. The unlocked output is a plaintext file; secure deletion, trusted boot, rollback protection and protection against privileged host access are absent. This experimental envelope is not LUKS2, disk-sector encryption or a production format; no automatic boot unlock is present. Anyone with the public key can construct a new valid envelope, so authenticity of the volume publisher requires a separate signature/trust mechanism.

```bash
python3 tools/vault.py keygen vault-private.pem vault-public.pem
python3 tools/vault.py seal build/vault.ext4 vault-public.pem demo.swvault
python3 tools/vault.py unlock demo.swvault vault-private.pem unlocked.ext4
```

Use a disposable test directory. `unlock` prompts for the passphrase; wrong credentials or tampering produce no output. Keep keys and plaintext outside Git. Do not use real secrets.

## Required final disk architecture (not implemented)
| Partition | Purpose | Protection |
|---|---|---|
| EFI | SwampBoot UEFI loader | Signed and measured; firmware trust anchor |
| LOGIN | Separate SwampLogin environment | Signed and verified, read-only during normal operation, offline |
| ROOT | EXT4 inside encrypted block device | Locked until user authentication |
| HOME | EXT4 inside independently encrypted block device | Locked until user authentication |
| RECOVERY | Minimal recovery environment | Signed, user-authenticated unlock only |

Preboot login is not an ordinary desktop lock screen. Verify SwampLogin before it receives credentials; no TPM-only automatic unlock. Release keys only after user authentication, then mount volumes and start the kernel/session. AES-XTS block encryption with independently verified integrity scheme or an established compatible authenticated block format, RSA-OAEP key wrapping where appropriate, Argon2id, TPM 2.0 sealing with user participation, signed boot policy, secure recovery and rollback protection remain requirements. Encrypted swap, secure logout, key eviction and power-state behavior must be validated. Do not claim cryptographic protection of UEFI NVRAM itself; protect SwampBoot's own registry and validate chainloaded OS loaders. No actual login partition, boot chain, encrypted ROOT/HOME, kernel crypto or installer in v0.6.0.
