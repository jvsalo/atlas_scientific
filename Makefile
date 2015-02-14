all: atsci_ph atsci_ec atsci_do

atsci_ph: atsci_ph.cpp
	g++ -Wall -Wextra -std=c++98 atsci_ph.cpp -o atsci_ph

atsci_ec: atsci_ec.cpp
	g++ -Wall -Wextra -std=c++98 atsci_ec.cpp -o atsci_ec

atsci_do: atsci_do.cpp
	g++ -Wall -Wextra -std=c++98 atsci_do.cpp -o atsci_do

