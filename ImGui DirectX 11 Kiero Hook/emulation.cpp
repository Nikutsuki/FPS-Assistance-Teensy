#include "emulation.h"

int Emulation::init()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return -1;
    }

    clientSocket1 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket1 == INVALID_SOCKET) {
        std::cerr << "Error creating socket: " << WSAGetLastError() << '\n';
        WSACleanup();
        return -1;
    }
    clientSocket2 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket2 == INVALID_SOCKET) {
		std::cerr << "Error creating socket: " << WSAGetLastError() << '\n';
		WSACleanup();
		return -1;
	}

	sockaddr_in serverAddr1 = { 0 };
    serverAddr1.sin_family = AF_INET;
    serverAddr1.sin_port = htons(42069);
    serverAddr1.sin_addr.s_addr = inet_addr("191.167.0.1");

	sockaddr_in serverAddr2 = { 0 };
    serverAddr2.sin_family = AF_INET;
    serverAddr2.sin_port = htons(42068);
    serverAddr2.sin_addr.s_addr = inet_addr("191.167.0.1");

    if (connect(clientSocket1, (SOCKADDR*)&serverAddr1, sizeof(serverAddr1)) != 0) {
        std::cerr << "Failed to connect to server\n";
        closesocket(clientSocket1);
        WSACleanup();
        return -1;
    }
    if (connect(clientSocket2, (SOCKADDR*)&serverAddr2, sizeof(serverAddr2)) != 0) {
        std::cerr << "Failed to connect to server\n";
        closesocket(clientSocket2);
        WSACleanup();
        return -1;
    }

	return 0;
}

void Emulation::loop()
{
    while (true)
    {
        recv(clientSocket1, buffer1, sizeof(buffer1), 0);

        int16_t x = 0;
        int16_t y = 0;
        uint8_t buttons = 0;

        memcpy(&x, buffer1, sizeof(int16_t));
        memcpy(&y, buffer1 + sizeof(int16_t), sizeof(int16_t));
        memcpy(&buttons, buffer1 + sizeof(int16_t) * 2, sizeof(uint8_t));

        const bool trigger = buttons & 8;
        const bool ads = buttons & 2;
        const bool left_click = buttons & 1;
        const bool side_button = buttons & 16;
        trigger_on = trigger;
        ads_on = ads;
		mouse_button_down = left_click;
		side_button_down = side_button;

		int x_aim_assist = x;
		int y_aim_assist = y;

		aim_assist->loop(x_aim_assist, y_aim_assist);

        x = x_aim_assist;
        y = y_aim_assist;

        if (trigger_should_shoot)
        {
			buttons |= 1;
			trigger_last_time = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count());
        }

        if (trigger_should_reset)
        {
			buttons &= ~1;
			trigger_should_reset = 0;
            trigger_should_shoot = 0;
		}

        memcpy(buffer1, &x, sizeof(int16_t));
        memcpy(buffer1 + sizeof(int16_t), &y, sizeof(int16_t));
        memset(buffer1 + sizeof(int16_t) * 2, 0, sizeof(uint8_t));

        send(clientSocket1, buffer1, sizeof(buffer1), 0);
    }
}

void Emulation::shoot_trigger() noexcept
{
    if (trigger_on && trigger_should_shoot)
    {
        if (trigger_should_reset)
        {
            uint8_t buttons = 0;

            if (ads_on)
            {
                buttons |= 2;
            }

			if (side_button_down)
			{
				buttons |= 16;
			}

            memset(buffer2, 0, sizeof(int16_t));
            memset(buffer2 + sizeof(int16_t), 0, sizeof(int16_t));
            memcpy(buffer2 + sizeof(int16_t) * 2, &buttons, sizeof(uint8_t));
            send(clientSocket2, buffer2, sizeof(buffer2), 0);

            char idk = 0;
            recv(clientSocket2, &idk, sizeof(char), 0);
        }

        uint8_t buttons = 0;
        buttons |= 1;

        if (ads_on)
        {
            buttons |= 2;
        }

        if (side_button_down)
        {
			buttons |= 16;
        }

		memset(&buffer2[0], 0, sizeof(int16_t));
        memset(&buffer2[0] + sizeof(int16_t), 0, sizeof(int16_t));
        memcpy(&buffer2[0] + sizeof(int16_t) * 2, &buttons, sizeof(uint8_t));
		send(clientSocket2, &buffer2[0], sizeof(buffer2), 0);

        char idk = 0;
        recv(clientSocket2, &idk, sizeof(char), 0);

        Sleep(10);

        if (ads_on)
        {
			buttons |= 2;
		}

		if (side_button_down)
		{
            buttons |= 16;
		}

        buttons &= ~1;
        memset(buffer2, 0, sizeof(int16_t));
        memset(buffer2 + sizeof(int16_t), 0, sizeof(int16_t));
        memcpy(buffer2 + sizeof(int16_t) * 2, &buttons, sizeof(uint8_t));
        send(clientSocket2, buffer2, sizeof(buffer2), 0);

        recv(clientSocket2, &idk, sizeof(char), 0);
	}
}

void Emulation::aimbot2(int x, int y) noexcept
{
    int16_t x_send = 0;
    int16_t y_send = 0;
    uint8_t buttons = 0;

    x_send += x;
	y_send += y;

	if (mouse_button_down)
    {
		buttons |= 1;
	}

    if (ads_on)
    {
		buttons |= 2;
	}

	if (side_button_down)
	{
		buttons |= 16;
	}

    memcpy(buffer2, &x_send, sizeof(int16_t));
    memcpy(buffer2 + sizeof(int16_t), &y_send, sizeof(int16_t));
    memcpy(buffer2 + sizeof(int16_t) * 2, &buttons, sizeof(uint8_t));
    send(clientSocket2, buffer2, sizeof(buffer2), 0);
    char idk = 0;
    recv(clientSocket2, &idk, sizeof(char), 0);
}
