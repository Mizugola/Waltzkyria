#include <SFML/Network.hpp>
#include <iostream>
#include <chrono>
#include <thread>

#include "Functions.hpp"

void fillAddress(std::vector<std::string>& pool, std::string& addressList, std::string& clientAddress, int port)
{
    if (addressList != "") {
        std::vector<std::string> clientAddressPool = Functions::String::split(addressList, ",");
        for (std::string address : clientAddressPool) {
            if (Functions::String::contains(address, "!")) {
                std::cout << "    Peering from " << Functions::String::replaceString(address, "!", clientAddress) << std::endl;
            }
        }
        for (std::string& address : Functions::String::split(addressList, ",")) {
            Functions::String::replaceStringInPlace(address, "localhost", "127.0.0.1");
            Functions::String::replaceStringInPlace(address, "!", clientAddress);
            if (!Functions::Vector::isInList(address, pool) && address != ("127.0.0.1:" + std::to_string(port))) {
                std::cout << "        Client " << address << std::endl;
                std::cout << ("127.0.0.1:" + std::to_string(port)) << " != " << address << std::endl;
                pool.push_back(address);
            }
        }
    }
}

int main(int argc, char** argv)
{
    std::cout << "Waltzkyria Node" << std::endl;
    Functions::Run::Parser runArgs(argv, argc);
    int port = 0;
    std::vector<std::string> addressPool;
    std::map<std::string, int> expirationPool;
    if (runArgs.argumentExists("-p")) {
        port = std::stoi(runArgs.getArgumentValue("-p"));
    }
    if (runArgs.argumentExists("-a")) {
        addressPool = Functions::String::split(runArgs.getArgumentValue("-a"), ",");
    }

    sf::TcpSocket clientBridge;
    sf::TcpListener serverBridge;
    sf::TcpSocket currentClient;
    clientBridge.setBlocking(false);
    serverBridge.setBlocking(false);
    currentClient.setBlocking(false);
    serverBridge.listen(port);
    port = serverBridge.getLocalPort();
    sf::Packet sender;
    sf::Packet receiver;

    std::cout << "Running instance on port : " << port << std::endl;

    while (true) {
        if (serverBridge.accept(currentClient) == sf::Socket::Done) {
            std::cout << "AsServer" << std::endl;
            currentClient.receive(receiver);
            std::string cliList;
            receiver >> cliList;
            std::cout << "    ReceiveAsServer ::" << cliList << std::endl;
            std::string cliAddress = currentClient.getRemoteAddress().toString();
            fillAddress(addressPool, cliList, cliAddress, port);

            std::cout << "    SeedingAsServer to " << cliAddress << std::endl;
            std::vector<std::string> sendAddressPool = addressPool;
            sendAddressPool.push_back("!:" + std::to_string(port));
            sender << Functions::Vector::join(sendAddressPool, ",");
            currentClient.send(sender);
            serverBridge.close();
            serverBridge.listen(port);
        }
        else {
            serverBridge.close();
            serverBridge.listen(port);
        }
        int addressIndex = 0;
        std::cout << "AsClient" << std::endl;
        for (std::string& address : addressPool) {
            std::cout << "    Trying to connect to : " << address << std::endl;
            if (clientBridge.connect(Functions::String::split(address, ":")[0], std::stoi(Functions::String::split(address, ":")[1])) == sf::Socket::Done) {
                std::cout << "    SeedingAsClient to " << Functions::String::split(address, ":")[0] << ":" << std::stoi(Functions::String::split(address, ":")[1]) << std::endl;
                std::vector<std::string> sendAddressPool = addressPool;
                sendAddressPool.push_back("!:" + std::to_string(port));
                sender << Functions::Vector::join(sendAddressPool, ",");
                clientBridge.send(sender);

                std::string cliAddress = clientBridge.getRemoteAddress().toString();
                clientBridge.receive(receiver);
                std::string cliList;
                receiver >> cliList;
                std::cout << "    ReceiveAsClient ::" << cliList << std::endl;
                if (expirationPool.find(address) != expirationPool.end()) {
                    expirationPool[address] = 3;
                }
                fillAddress(addressPool, cliList, cliAddress, port);
                
                /*if (message != "") {
                    std::cout << "Message : " << message << std::endl;
                    expirationPool[address] = 60;
                }
                else {
                    expirationPool[address]--;
                    if (expirationPool[address] <= 0) {
                        std::cout << "Address : " << address << " expired.." << std::endl;
                        addressPool.erase(addressPool.begin() + addressIndex);
                    }
                }*/  
            }
            addressIndex++;
        }
        std::cout << "Pool" << std::endl;
        int poolIndex = 0;
        for (std::string& addr : addressPool) {
            std::cout << "    Address : " << addr << std::endl;
            if (expirationPool.find(addr) != expirationPool.end()) {
                expirationPool[addr]--;
                if (expirationPool[addr] == 0) {
                    expirationPool.erase(expirationPool.find(addr));
                    addressPool.erase(addressPool.begin() + poolIndex);
                    std::cout << "        (Expired)" << std::endl;
                }
            } else {
                expirationPool[addr] = 3;
            }
            poolIndex++;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    }
}
