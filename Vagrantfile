# -*- mode: ruby -*-
# vi: set ft=ruby :

$preProvision= <<SCRIPT
sudo apt-get update && sudo apt-get install tmux python-pip pkg-config liblua5.3-dev -y
pip install pipenv
sudo mkdir -p /opt/wpn
sudo chown -R vagrant /opt/wpn
SCRIPT

$provision= <<SCRIPT
cd /vagrant
make
mv gameserver server
SCRIPT

$startServer= <<SCRIPT
tmux kill-session -t "wpn"
tmux new -d -n "server" -s "wpn" -c "/vagrant/server" "./gameserver"
SCRIPT

Vagrant.configure("2") do |config|
  config.vm.box = "bento/ubuntu-18.04"

  config.vm.network "forwarded_port", guest: 8080, host: 8080
  config.vm.network "forwarded_port", guest: 8090, host: 8090

  # Pre-provision
  config.vm.provision "shell", inline: $preProvision

  # Provisioning scripts
  config.vm.provision "shell", inline: $provision, privileged: false

  # Start server in tmux session (every reboot)
  config.vm.provision "shell", inline: $startServer, privileged: false, run: "always"
end
