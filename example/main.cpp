#include <iostream>
#include <fstream>

#include "hawkbit.hpp"

using namespace hawkbit;

const char *AUTH_CERT_PATH_ENV_NAME = "CERT_PATH";
const char *PROVISIONING_ENDPOINT_ENV_NAME = "PROVISIONING_ENDPOINT";
const char *X_APIG_TOKEN_ENV_NAME = "X_APIG_TOKEN";

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

    std::unique_ptr<Response> onDeploymentAction(std::unique_ptr<DeploymentBase> dp) override {
        std::cout << ">> Get Deployment base request" << std::endl;
        std::cout << " id: " << dp->getId() << " update: " << dp->getUpdateType() << std::endl;
        std::cout << " download: " << dp->getDownloadType() << " inWindow: " << (bool) dp->isInMaintenanceWindow()
                  << std::endl;

        auto builder = ResponseBuilder::newInstance();
        builder->addDetail("Printed deployment base info");
        std::cout << " + CHUNKS:" << std::endl;

        for (const auto &chunk: dp->getChunks()) {
            std::cout << "  part: " << chunk->getPart() << std::endl;
            std::cout << "  name: " << chunk->getName() << " version: " << chunk->getVersion() << std::endl;
            std::cout << "  + ARTIFACTS:" << std::endl;
            for (const auto &artifact: chunk->getArtifacts()) {
                std::cout << "   filename: " << artifact->getFilename() << " size: " << artifact->size() << std::endl;
                std::cout << "   md5: " << artifact->getFileHashes().md5 << std::endl;
                std::cout << "   sha1: " << artifact->getFileHashes().sha1 << std::endl;
                std::cout << "   sha256: " << artifact->getFileHashes().sha256 << std::endl;
                builder->addDetail(artifact->getFilename() + " described. Starting download ...");
                std::cout << "  .. downloading " + artifact->getFilename() + "...";
                artifact->downloadTo(artifact->getFilename());
                builder->addDetail("Downloaded " + artifact->getFilename());
                std::cout << "[OK]" << std::endl;
            }
            std::cout << " + ---------------------------" << std::endl;
        }

        return builder->addDetail("Work done. Sending response")
                ->setIgnoreSleep()
                ->setExecution(Response::CLOSED)
                ->setFinished(Response::SUCCESS)
                ->setResponseDeliveryListener(
                        std::shared_ptr<ResponseDeliveryListener>(new DeploymentBaseFeedbackDeliveryListener()))
                ->build();

    }

    std::unique_ptr<Response> onCancelAction(std::unique_ptr<CancelAction> action) override {
        std::cout << ">> CancelAction: id " << action->getId() << ", stopId " << action->getStopId() << std::endl;

        return ResponseBuilder::newInstance()
                ->setExecution(hawkbit::Response::CLOSED)
                ->setFinished(hawkbit::Response::SUCCESS)
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

char *getEnvOrExit(const char *name) {
    auto env = std::getenv(name);
    if (env == nullptr) {
        std::cout << "Environment variable " << name << " not set" << std::endl;
        exit(2);
    }
    return env;
}

int main() {
    std::cout << "hawkBit-cpp client started..." << std::endl;

    auto clientCertificatePath = getEnvOrExit(AUTH_CERT_PATH_ENV_NAME);
    auto provisioningEndpoint = getEnvOrExit(PROVISIONING_ENDPOINT_ENV_NAME);
    // special variable for cloud
    auto xApigToken = getEnvOrExit(X_APIG_TOKEN_ENV_NAME);

    std::ifstream t((std::string(clientCertificatePath)));
    if (!t.is_open()) {
        std::cout << "File " << clientCertificatePath << " not exists" << std::endl;
    }
    std::string crt((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());

    auto builder = ClientBuilder::newInstance();
    builder->setCrt(crt)->setProvisioningEndpoint(std::string(provisioningEndpoint))
            ->setEventHandler(std::shared_ptr<EventHandler>(new Handler()))
            ->addProvisioningHeader("X-Apig-AppCode", std::string(xApigToken))
            ->build()->run();
}