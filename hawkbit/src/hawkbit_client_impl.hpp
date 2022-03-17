#pragma once

#include <string>
#include <memory>

#include "httplib.h"
#include "uriparse.hpp"
#include "hawkbit/hawkbit_event_handler.hpp"
#include "hawkbit/hawkbit_exceptions.hpp"
#include "actions_impl.hpp"
#include "hawkbit/hawkbit_client.hpp"

namespace hawkbit {

    struct PollingData_;

    class HawkbitCommunicationClient : public DownloadProvider, public Client, public AuthRestoreHandler {
    protected:
        uri::URI hawkbitURI;

        httplib::Headers defaultHeaders;

        std::shared_ptr<EventHandler> handler;
        std::shared_ptr<AuthErrorHandler> authErrorHandler;

        int defaultSleepTime;
        int currentSleepTime;

        bool ignoreSleep;

        bool serverCertificateVerify = true;

        struct {
            std::string crt;
            std::string key;
            bool isSet = false;
        } mTLSKeypair;

        // starting hawkbit communication logic.
        //  returns execute time in ms
        void doPoll();

        // call user-defined handler and send config data to hawkBit
        void followConfigData(uri::URI &);

        // call user-defined handler to process cancelAction and send response to hawkBit
        void followCancelAction(uri::URI &);

        // call user-defined handler to process deploymentBase and send response to hawkBit
        void followDeploymentBase(uri::URI &);

        // all requests should go via retryHandler
        httplib::Result wrappedRequest(uri::URI, const std::function<httplib::Result(httplib::Client &)> &);

        httplib::Result retryHandler(uri::URI, const std::function<httplib::Result(httplib::Client &)> &);

        // creates httpClient with predefined params
        httplib::Client newHttpClient(uri::URI &) const;

    public:

        [[noreturn]] virtual void run() override;

        void downloadTo(uri::URI uri, const std::string &path) override;

        std::string getBody(uri::URI uri) override;

        void downloadWithReceiver(uri::URI uri, std::function<bool(const char *, size_t)> function) override;

        void setTLS(const std::string &crt, const std::string &key) override;

        void setTargetEndpoint(std::string &endpoint) override;

        void setDeviceToken(const std::string &string) override;

        void setGatewayToken(const std::string &string) override;

        friend class DefaultClientBuilderImpl;
    };

    class DefaultClientBuilderImpl : public DefaultClientBuilder {
        uri::URI hawkbitUri;

        int pollingTimeout = 30000;

        httplib::Headers defaultHeaders;

        std::shared_ptr<AuthErrorHandler> authErrorHandler;
        std::shared_ptr<EventHandler> handler;

        std::string token;
        std::string crt, key;

        enum AuthorizeVariants {
            NOT_SET,
            GATEWAY_TOKEN,
            DEVICE_TOKEN,
            M_TLS_KEYPAIR
        };

        bool verifyServerCertificate = true;

        AuthorizeVariants authVariant = AuthorizeVariants::NOT_SET;

    public:
        DefaultClientBuilder *setHawkbitEndpoint(const std::string &) override;

        DefaultClientBuilder *setDefaultPollingTimeout(int pollingTimeout_) override;

        DefaultClientBuilder *setEventHandler(std::shared_ptr<EventHandler> handler) override;

        DefaultClientBuilder *addHeader(const std::string &, const std::string &) override;

        DefaultClientBuilder *setGatewayToken(const std::string &) override;

        DefaultClientBuilder *setDeviceToken(const std::string &) override;

        DefaultClientBuilder *notVerifyServerCertificate() override;


        DefaultClientBuilder *setTLS(const std::string &crt, const std::string &key) override;

        DefaultClientBuilder *setAuthErrorHandler(std::shared_ptr<AuthErrorHandler>) override;

        DefaultClientBuilder *setHawkbitEndpoint(const std::string &endpoint,
                                                 const std::string &controllerId_, const std::string &tenant_ = "default") override;

        std::unique_ptr<Client> build() override;
    };

}