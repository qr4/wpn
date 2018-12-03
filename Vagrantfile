Vagrant.configure("2") do |config|
  config.vm.box = "bento/ubuntu-18.04"

  # Create a private network, which allows host-only access to the machine
#  config.vm.network "private_network", ip: "10.9.8.7"

  config.vm.network "forwarded_port", guest: 5000, host: 5000

  # Start server in tmux session (every reboot)
#  config.vm.provision "shell", inline: $startServer, privileged: false,
#                      run: "always"
end
