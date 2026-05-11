## Outbound Filter
Parses filter rules and sends them to the [lkm-outbound-filter](https://github.com/paper209/lkm-outbound-filter) over UDP

## Supported Filters
- **Signature Filter** (TCP, UDP, ICMP)  
   Filters packets by inspecting payloads and matching specific signatures (DPI).

- **Port Filter** (TCP, UDP)  
   Filters traffic based on destination ports.

- **Netmask Filter**  (ALL)  
   Filters traffic by matching destination addresses against configured netmasks.

## Filter Format
- **Signature Filter**   
  `signature:binary_path`   
- **Port Filter**   
  `port:protocol:port`   
- **Netmask Filter**   
  `netmask:network/mask`

## Example
example.filter:
```
signature:test_signatrue
port:udp:8080
netmask:1.1.1.1/24
```
