#pragma once

#include <QuasarEngine/JSON/JsonWriter.h>
#include <QuasarEngine/JSON/JsonParser.h>

#include <QuasarEngine/Core/Logger.h>

//#include <QuasarEngine/Random/PerlinNoise.h>

//#define STB_IMAGE_WRITE_IMPLEMENTATION
//#include <stb_image_write.h>

#include <QuasarEngine/Blockchain/BlockchainManager.h>

#include <QuasarEngine/Crypto/CryptoManager.h>

#include "ItemBlockEvent.h"

class GlfwEditorLayer : public Layer
{
public:
	GlfwEditorLayer()
	{
        /*auto obj = std::make_shared<JsonObject>();
        obj->add("name", std::make_shared<JsonPrimitive>("Alice"));
        obj->add("age", std::make_shared<JsonPrimitive>(30));
        obj->add("isStudent", std::make_shared<JsonPrimitive>(false));

        auto arr = std::make_shared<JsonArray>();
        arr->add(std::make_shared<JsonPrimitive>("C++"));
        arr->add(std::make_shared<JsonPrimitive>("Python"));
        obj->add("languages", arr);

        try {
            JsonWriter writer("output.json");
            writer.write(obj);
            std::cout << "Fichier JSON sauvegarde avec succès!" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Erreur lors de l'ecriture du fichier : " << e.what() << std::endl;
        }

        std::string jsonStr = R"({
            "name": "Alice",
            "age": 25,
            "isStudent": false,
            "grades": [85, 90, 78],
            "address": {
                "city": "Paris",
                "zip": "75000"
            }
        })";

        std::cout << "Original JSON: " << jsonStr << std::endl;

        try {
            JsonParser parser(jsonStr);
            auto jsonObj = parser.parse();
            std::cout << jsonObj->toString() << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Erreur de parsing : " << e.what() << std::endl;
        }

        Logger logger;
        logger.log(Logger::Level::INFO_LOG, "This is an info message.");
        logger.log(Logger::Level::WARNING_LOG, "This is a warning message.");
        logger.log(Logger::Level::ERROR_LOG, "This is an error message.");
        logger.log(Logger::Level::DEBUG_LOG, "This is a debug message.");*/

        /*int width = 2048;
        int height = 2048;
        int seed = 12345;

        PerlinNoise perlin(seed);

        std::vector<unsigned char> image(width * height * 3);

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                float noiseValue = perlin.noise((float)x / width, (float)y / height);

                int pixelValue = static_cast<int>((noiseValue + 1.0f) * 127.5f);

                int idx = (y * width + x) * 3;
                image[idx] = pixelValue;
                image[idx + 1] = pixelValue;
                image[idx + 2] = pixelValue;
            }
        }

        stbi_write_png("perlin_noise_stb.png", width, height, 3, image.data(), width * 3);

        std::cout << "Image generee et sauvegardee sous 'perlin_noise_stb.png'" << std::endl;*/

        CryptoManager::instance().Init_SHA256();

        std::array<uint8_t, 16> key = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x97, 0x75, 0x46, 0x40, 0x09, 0x6c };
        CryptoManager::instance().Init_AES(key);

        /*BlockchainManager manager;
        manager.addBlockchain();

        Blockchain& playerBlockchain = manager.getBlockchain(0);

        playerBlockchain.addBlock({ ItemBlockEvent("Sword", "UPGRADE", 5) });
        playerBlockchain.addBlock({ ItemBlockEvent("Armor", "DAMAGE_TAKEN", 20) });
        playerBlockchain.addBlock({ ItemBlockEvent("Monster", "KILL_MOB", 100) });
        playerBlockchain.addBlock({ ItemBlockEvent("Potion", "DRINK_POTION", 20) });
        playerBlockchain.addBlock({ ItemBlockEvent("Coin", "EARN_COIN", 1) });

        playerBlockchain.printBlockchain();*/

        /*std::string message = "Hello, AES-256-CBC!";
        std::vector<uint8_t> plaintext(message.begin(), message.end());

        std::vector<uint8_t> encrypted = CryptoManager::instance().AES_Encrypt(plaintext);
        CryptoManager::PrintHex("Encrypted", encrypted);

        std::vector<uint8_t> decrypted = CryptoManager::instance().AES_Decrypt(encrypted);
        std::string decryptedMessage(decrypted.begin(), decrypted.end());
        std::cout << "Decrypted: " << decryptedMessage << std::endl;*/
	}

	~GlfwEditorLayer() = default;

	void OnAttach() override
	{

	}

	void OnDetach() override
	{

	}

	void OnUpdate(double dt) override
	{

	}
};