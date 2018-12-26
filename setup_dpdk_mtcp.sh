GREEN='\033[0;32m'
NC='\033[0m'

# First download dpdk
if [ -z "$(ls -A $PWD/dpdk)" ]; then
    printf "${GREEN}Cloning dpdk...\n $NC"
    git submodule init
    git submodule update
fi

# Setup dpdk source for compilation
export RTE_SDK=$PWD/dpdk
export RTE_TARGET=x86_64-native-linuxapp-gcc
if grep "ldflags.txt" $RTE_SDK/mk/rte.app.mk > /dev/null
then
    :
else
    sed -i -e 's/O_TO_EXE_STR =/\$(shell if [ \! -d \${RTE_SDK}\/\${RTE_TARGET}\/lib ]\; then mkdir \${RTE_SDK}\/\${RTE_TARGET}\/lib\; fi)\nLINKER_FLAGS = \$(call linkerprefix,\$(LDLIBS))\n\$(shell echo \${LINKER_FLAGS} \> \${RTE_SDK}\/\${RTE_TARGET}\/lib\/ldflags\.txt)\nO_TO_EXE_STR =/g' $RTE_SDK/mk/rte.app.mk
fi

# Compile dpdk and configure system
cd dpdk
make install T=$RTE_TARGET
cd ..

# Print the user message
printf "Set ${GREEN}RTE_SDK$NC env variable as $RTE_SDK\n"
printf "Set ${GREEN}RTE_TARGET$NC env variable as $RTE_TARGET\n"

# Compile mtcp
printf "Compile ${GREEN}mtcp$NC libraries"
cd mtcp
./configure --with-dpdk-lib=$RTE_SDK/$RTE_TARGET
make

# Create interfaces
printf "Creating ${GREEN}mtcp-dpdk$NC interface entries\n"
cd dpdk-iface-kmod
make
cd ../../
