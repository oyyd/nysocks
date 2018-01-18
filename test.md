nysocks <command>

Commands:
  nysocks server  Start a tunnel server.
  nysocks client  Start a tunnel client.

Options:
  --version                Show version number                         [boolean]
  --config, -c             The path of a json file that describe your
                           configuration.
  --daemon, -d             Run with a daemon(pm2): start, stop, restart.
  --daemon_status, -s      Show daemoned(pm2) processes status
  --mode, -m               Like kcptun: normal, fast, fast2, fast3.
  --password, -k           The passowrd/key for the encryption of transmissio.
  --socket_amount          The amount of connections to be created for each
                           client (default: 10)
  --server_addr, -a        The host of your server.
  --server_port, -p        The port of your server.
  --client_protocol, --cp  The protocol that will be used by clients: SS, SOCKS
                           (default: SOCKS)
  --socks_port             Specify the local port for SOCKS service (default:
                           1080)
  --ss_port                Specify the local port for ssServer service (default:
                           8083)
  --ss_password            Specify the key for the encryption of ss
  --ss_method              Specify the method of the encryption for ss (default:
                           aes-128-cfb)
  --log_path               The file path for logging. If not set, will log to
                           the console.
  --log_memory             Log memory info.
  --log_conn               Log connections info.
  --help                   Show help                                   [boolean]

