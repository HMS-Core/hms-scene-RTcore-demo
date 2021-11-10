/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: Utils implementation.
 */

#include "Utils.h"

#include "Log.h"
#include <memory>
#include <fstream>

namespace Detail {
class ScopedFile {
public:
#ifdef __ANDROID__
    static std::unique_ptr<ScopedFile> Open(const std::string &filePath, AAssetManager *assetManager)
    {
        if (assetManager == nullptr) {
            return nullptr;
        }

        AAsset *assetFile = nullptr;
        assetFile = AAssetManager_open(assetManager, filePath.c_str(), AASSET_MODE_BUFFER);

        return std::make_unique<ScopedFile>(assetFile);
    }

    ScopedFile(AAsset *inFile) : m_assetFile(inFile) {}

    ~ScopedFile() noexcept
    {
        AAsset_close(m_assetFile);
    }

    const std::string Read() const
    {
        auto fileLength = AAsset_getLength(m_assetFile);
        if (fileLength <= 0) {
            return {};
        }

        std::string fileContent(fileLength, 0);
        AAsset_read(m_assetFile, &fileContent[0], fileLength);
        return fileContent;
    }

private:
    AAsset *m_assetFile = nullptr;
#else
    static std::unique_ptr<ScopedFile> Open(const std::string &filePath)
    {
        std::ifstream inFile = std::ifstream(filePath, std::ios::binary | std::ios::in | std::ios::ate);
        if (!inFile.is_open()) {
            LOGE("%s, file \" %s \" not exists.", __func__, filePath.c_str());
            return nullptr;
        }

        return std::make_unique<ScopedFile>(std::move(inFile));
    }

    const std::string Read()
    {
        auto fileLength = m_inFile.tellg();
        if (fileLength <= 0) {
            return {};
        }

        std::string fileContent(fileLength, 0);
        m_inFile.seekg(0, std::ios::beg);
        m_inFile.read(&fileContent[0], fileLength);
        return fileContent;
    }

    explicit ScopedFile(std::ifstream &&inFile) : m_inFile(std::move(inFile)) {}

    ~ScopedFile() noexcept
    {
        m_inFile.close();
    }

private:
    std::ifstream m_inFile;
#endif
};
} // namespace Detail

namespace Utils {
#ifdef __ANDROID__
std::string GetFileContent(const std::string &fileName, AAssetManager *assetManager)
{
    auto f = Detail::ScopedFile::Open(fileName, assetManager);
#else
std::string GetFileContent(const std::string &fileName)
{
    auto f = Detail::ScopedFile::Open(fileName);
#endif // __ANDROID__
    if (f == nullptr) {
        return {};
    }

    return f->Read();
}
} // namespace Utils
