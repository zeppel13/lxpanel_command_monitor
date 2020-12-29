all: commander.c cmdspawn.c cmdspawn.h
	gcc -Wall `pkg-config --cflags gtk+-2.0 lxpanel` -shared -fPIC commander.c -o commander.so `pkg-config --libs lxpanel gtk+-2.0 `
install: all
	sudo cp ./commander.so /usr/lib/lxpanel/plugins/commander.so
run: install all
	lxpanelctl restart
	@echo "add new plugin to lxpanel"
help:
	@echo "all | install | run | help"
	@echo "see for yourself on"
	@echo "https://wiki.lxde.org/en/How_to_write_plugins_for_LXPanel"
test_spawncmd:
	gcc -Wall `pkg-config --cflags gtk+-2.0` main.c cmdspawn.c `pkg-config gtk+-2.0 --libs` && ./a.out
