if [ ! -d "bin" ]; then
    mkdir bin
else
	rm bin/*
fi

g++ -g -O0 -I . -o bin/interrupts_EP interrupts_RileyFoxton_EstebanHeidrich_EP.cpp
g++ -g -O0 -I . -o bin/interrupts_RR interrupts_RileyFoxton_EstebanHeidrich_RR.cpp
g++ -g -O0 -I . -o bin/interrupts_EP_RR interrupts_RileyFoxton_EstebanHeidrich_EP_RR.cpp