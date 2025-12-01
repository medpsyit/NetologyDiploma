#pragma once 

#include <string>

enum class ProtocolType
{
	HTTP = 0,
	HTTPS = 1,
    UNKNOWN = 2
};

struct Link
{
	ProtocolType protocol;
	std::string hostName;
	std::string query;

	bool operator==(const Link& l) const
	{
		return protocol == l.protocol
			&& hostName == l.hostName
			&& query == l.query;
	}

    static Link parse(const std::string& url) {
        Link link;
        std::string protocolStr;

        // Определение протокола
        if (url.find("https://") == 0) {
            link.protocol = ProtocolType::HTTPS;
            protocolStr = "https://";
        }
        else if (url.find("http://") == 0) {
            link.protocol = ProtocolType::HTTP;
            protocolStr = "http://";
        }
        else {
            link.protocol = ProtocolType::UNKNOWN;
            throw std::runtime_error ("Unknown protocol! Check the link.");
        }

        // Удаление протокола
        std::string trimmedUrl = url.substr(protocolStr.length());

        // Определение позиции знака вопроса для получения хоста и запроса
        size_t queryPos = trimmedUrl.find('/');
        if (queryPos != std::string::npos) {
            link.hostName = trimmedUrl.substr(0, queryPos);
            link.query = trimmedUrl.substr(queryPos);
        }
        else {
            link.hostName = trimmedUrl;
            link.query = "/";
        }

        return link;
    }
};

