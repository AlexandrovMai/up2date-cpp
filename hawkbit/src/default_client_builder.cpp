#include "hawkbit/hawkbit_client.hpp"
#include "hawkbit_client_impl.hpp"
#include "uriparse.hpp"

namespace hawkbit {
    std::unique_ptr<DefaultClientBuilder> DefaultClientBuilder::newInstance() {
        return std::unique_ptr<DefaultClientBuilder>(new DefaultClientBuilderImpl());
    }

    DefaultClientBuilder *DefaultClientBuilderImpl::setEventHandler(std::shared_ptr<EventHandler> handler_) {
        handler = handler_;

        return this;
    }

    DefaultClientBuilder *DefaultClientBuilderImpl::setHawkbitEndpoint(const std::string &endpoint) {
        this->hawkbitUri = uri::URI::fromString(endpoint);

        return this;
    }

    DefaultClientBuilder *DefaultClientBuilderImpl::setHawkbitEndpoint(const std::string &endpoint,
                                                                       const std::string &controllerId_,
                                                                       const std::string &tenant_) {
        auto hawkbitEndpoint = uri::URI::fromString(endpoint);
        this->hawkbitUri = uri::URI::fromString(
                hawkbitEndpoint.getScheme() + "://" + hawkbitEndpoint.getAuthority() + "/" + tenant_ + "/controller/v1/" +
                controllerId_);

        return this;
    }

    DefaultClientBuilder *DefaultClientBuilderImpl::setDefaultPollingTimeout(int pollingTimeout_) {
        pollingTimeout = pollingTimeout_;

        return this;
    }

    DefaultClientBuilder *DefaultClientBuilderImpl::addHeader(const std::string &k, const std::string &v) {
        defaultHeaders.insert({k, v});

        return this;
    }

    DefaultClientBuilder *DefaultClientBuilderImpl::setGatewayToken(const std::string &token_) {
        if (authVariant != AuthorizeVariants::NOT_SET) {
            throw std::runtime_error("Another authority type is already set");
        }

        token = token_;
        authVariant = AuthorizeVariants::GATEWAY_TOKEN;

        return this;
    }

    DefaultClientBuilder *DefaultClientBuilderImpl::setDeviceToken(const std::string &token_) {
        if (authVariant != AuthorizeVariants::NOT_SET) {
            throw std::runtime_error("Another authority type is already set");
        }

        token = token_;
        authVariant = AuthorizeVariants::DEVICE_TOKEN;

        return this;
    }

    DefaultClientBuilder *DefaultClientBuilderImpl::setTLS(const std::string &crt_, const std::string &key_) {
        if (authVariant != AuthorizeVariants::NOT_SET) {
            throw std::runtime_error("Another authority type is already set");
        }
        crt = crt_;
        key = key_;
        authVariant = AuthorizeVariants::M_TLS_KEYPAIR;

        return this;
    }

    DefaultClientBuilder *DefaultClientBuilderImpl::notVerifyServerCertificate() {
        verifyServerCertificate = false;

        return this;
    }


    DefaultClientBuilder *DefaultClientBuilderImpl::setAuthErrorHandler(std::shared_ptr<AuthErrorHandler> e) {
        authErrorHandler = e;

        return this;
    }

    std::unique_ptr<Client> DefaultClientBuilderImpl::build() {
        auto cli = new HawkbitCommunicationClient();
        auto cliPtr = std::unique_ptr<Client>(cli);

        cli->hawkbitURI = hawkbitUri;
        cli->defaultSleepTime = pollingTimeout;
        cli->currentSleepTime = pollingTimeout;
        cli->handler = handler;
        cli->defaultHeaders = defaultHeaders;
        cli->serverCertificateVerify = verifyServerCertificate;
        cli->authErrorHandler = authErrorHandler;

        if (authVariant == AuthorizeVariants::M_TLS_KEYPAIR) {
            cli->setTLS(crt, key);
        } else if (authVariant == AuthorizeVariants::GATEWAY_TOKEN) {
            cli->setGatewayToken(token);
        } else if (authVariant == AuthorizeVariants::DEVICE_TOKEN) {
            cli->setDeviceToken(token);
        }

        return cliPtr;
    }

}