CC = gcc
CFLAGS = -std=c23 -Wall -Werror -DUNICODE -D_UNICODE -masm=intel
LDFLAGS = -static -municode -mwindows

SRCDIR := src
BUILDDIR := build
TARGET := bin/csnake32
SRCEXT := c

SRCS := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SRCS:.$(SRCEXT)=.o))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@	

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@	

clean: 
	@$(RM) -r $(BUILDDIR) $(TARGET)

.PHONY: clean