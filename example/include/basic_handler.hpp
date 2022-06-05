#include <iostream>
#include <fstream>
#include <sys/stat.h>

#include "ddi.hpp"

using namespace ddi;

char *getEnvOrExit(const char *name) {
    auto env = std::getenv(name);
    if (env == nullptr) {
        std::cout << "Environment variable " << name << " not set" << std::endl;
        exit(2);
    }
    return env;
}

const auto INSTALLATION_SCRIPT = std::string(getEnvOrExit("INSTALLATION_SCRIPT"));


class CancelActionFeedbackDeliveryListener : public ResponseDeliveryListener {
public:
    void onSuccessfulDelivery() override {
        std::cout << ">> Successful delivered cancelAction response" << std::endl;
    }

    void onError() override {
        std::cout << ">> Error delivery cancelAction response" << std::endl;
    }
};

class DeploymentBaseFeedbackDeliveryListener : public ResponseDeliveryListener {
public:
    void onSuccessfulDelivery() override {
        std::cout << ">> Successful delivered deliveryAction response" << std::endl;
    }

    void onError() override {
        std::cout << ">> Error delivery deliveryAction response" << std::endl;
    }
};

struct CommandResult {
    std::string output;
    int exitstatus;

    friend std::ostream &operator<<(std::ostream &os, const CommandResult &result) {
        os << "command exitstatus: " << result.exitstatus << " output: " << result.output;
        return os;
    }
    bool operator==(const CommandResult &rhs) const {
        return output == rhs.output &&
               exitstatus == rhs.exitstatus;
    }
    bool operator!=(const CommandResult &rhs) const {
        return !(rhs == *this);
    }
};


class Command {

public:
    static CommandResult exec(const std::string &command) {
        int exitcode = 255;
        std::array<char, 1048576> buffer {};
        std::string result;
#ifdef _WIN32
        #define popen _popen
#define pclose _pclose
#define WEXITSTATUS
#endif
        FILE *pipe = popen(command.c_str(), "r");
        if (pipe == nullptr) {
            throw std::runtime_error("popen() failed!");
        }
        try {
            std::size_t bytesread;
            while ((bytesread = fread(buffer.data(), sizeof(buffer.at(0)), sizeof(buffer), pipe)) != 0) {
                auto res = std::string(buffer.data(), bytesread);
                result += res;
                std::cout << res;
            }
        } catch (...) {
            pclose(pipe);
            throw;
        }
        std::cout << std::endl;
        exitcode = WEXITSTATUS(pclose(pipe));
        return CommandResult{result, exitcode};
    }
};


class Handler : public EventHandler {
public:
    std::unique_ptr<ConfigResponse> onConfigRequest() override {
        std::cout << ">> Sending Config Data" << std::endl;

        return ConfigResponseBuilder::newInstance()
                ->addData("some", "config1")
                ->addData("some1", "new config")
                ->addData("some2", "RITMS123")
                ->addData("some3", "TES_TEST_TEST")
                ->setIgnoreSleep()
                ->build();
    }

    std::unique_ptr<Response> processInstall(std::unique_ptr<DeploymentBase> dp) {
        std::cout << ">> Get Deployment base request" << std::endl;
        std::cout << " id: " << dp->getId() << " update: " << dp->getUpdateType() << std::endl;
        std::cout << " download: " << dp->getDownloadType() << " inWindow: " << (bool) dp->isInMaintenanceWindow()
                  << std::endl;
        auto builder = ResponseBuilder::newInstance();
        std::cout << " + CHUNKS:" << std::endl;

        for (const auto &chunk: dp->getChunks()) {
            std::cout << "  part: " << chunk->getPart() << std::endl;
            std::cout << "  name: " << chunk->getName() << " version: " << chunk->getVersion() << std::endl;

            auto chunkPath = chunk->getVersion() + "@" + chunk->getName();
            if (mkdir(chunkPath.c_str(), 0744) != 0)
                throw std::runtime_error("Cannot create directory: " + chunkPath);

            std::cout << "  + ARTIFACTS:" << std::endl;
            for (const auto &artifact: chunk->getArtifacts()) {
                std::cout << "   filename: " << artifact->getFilename() << " size: " << artifact->size() << std::endl;
                std::cout << "   md5: " << artifact->getFileHashes().md5 << std::endl;
                std::cout << "   sha1: " << artifact->getFileHashes().sha1 << std::endl;
                std::cout << "   sha256: " << artifact->getFileHashes().sha256 << std::endl;
                auto artifactPath =  chunkPath + "/" + artifact->getFilename();

                std::cout << "  .. downloading " + artifactPath + "...";
                artifact->downloadTo(artifactPath);
                std::cout << "[OK]" << std::endl;
            }
            std::cout << " + ---------------------------" << std::endl;
        }


        auto output = Command::exec("python3 " + INSTALLATION_SCRIPT + " .");

        auto finished = (output.exitstatus == 0) ? Response::SUCCESS : Response::FAILURE;

        return builder->addDetail(output.output)
                ->setIgnoreSleep()
                ->setExecution(Response::CLOSED)
                ->setFinished(finished)
                ->setResponseDeliveryListener(
                        std::shared_ptr<ResponseDeliveryListener>(new DeploymentBaseFeedbackDeliveryListener()))
                ->build();
    }

    std::unique_ptr<Response> onDeploymentAction(std::unique_ptr<DeploymentBase> dp) override {
        try {
            return processInstall(std::move(dp));
        } catch (std::exception &e) {
            auto builder = ResponseBuilder::newInstance();
            return builder->addDetail("Got exception at installing package")
                    ->setIgnoreSleep()
                    ->addDetail("Exception:")
                    ->addDetail(e.what())
                    ->setExecution(Response::CLOSED)
                    ->setFinished(Response::FAILURE)
                    ->setResponseDeliveryListener(
                            std::shared_ptr<ResponseDeliveryListener>(new DeploymentBaseFeedbackDeliveryListener()))
                    ->build();
        }
    }

    std::unique_ptr<Response> onCancelAction(std::unique_ptr<CancelAction> action) override {
        std::cout << ">> CancelAction: id " << action->getId() << ", stopId " << action->getStopId() << std::endl;

        return ResponseBuilder::newInstance()
                ->setExecution(ddi::Response::CLOSED)
                ->setFinished(ddi::Response::SUCCESS)
                ->addDetail("Some feedback")
                ->addDetail("One more feedback")
                ->addDetail("Really important feedback")
                ->setResponseDeliveryListener(
                        std::shared_ptr<ResponseDeliveryListener>(new CancelActionFeedbackDeliveryListener()))
                ->setIgnoreSleep()
                ->build();
    }

    void onNoActions() override {
        std::cout << "No actions from hawkBit" << std::endl;
    }

    ~Handler() = default;

    Handler() = default;
};
