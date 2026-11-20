CC      := gcc
CFLAGS  := -std=gnu23 -Wall -Wextra -Werror -O2
TARGET  := lcd
SRCS    := $(wildcard *.c)
OBJS    := $(SRCS:.c=.o)
HDRS    := $(wildcard *.h)
PREFIX  := /usr/local/bin
SERVICE := /etc/systemd/system/$(TARGET).service

.PHONY: all install uninstall update monitor clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

install: $(TARGET)
	sudo install -m 755 $(TARGET) $(PREFIX)/$(TARGET)
	printf '[Unit]\nDescription=1602 LCD Clock Display Controller Daemon\nAfter=local-fs.target\n\n[Service]\nExecStart=%s/%s -d\nRestart=always\nRestartSec=2\n\n[Install]\nWantedBy=multi-user.target\n' "$(PREFIX)" "$(TARGET)" | sudo tee $(SERVICE) > /dev/null
	sudo systemctl daemon-reload
	sudo systemctl enable --now $(TARGET)

update: $(TARGET)
	sudo install -m 755 $(TARGET) $(PREFIX)/$(TARGET)
	sudo systemctl restart $(TARGET)

monitor:
	@watch -n 0.5 cat /dev/shm/lcd.screen

uninstall:
	-sudo systemctl disable --now $(TARGET)
	sudo rm -f $(SERVICE) $(PREFIX)/$(TARGET)
	sudo systemctl daemon-reload

clean:
	rm -f $(OBJS) $(TARGET)