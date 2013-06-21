/***
* ==++==
*
* Copyright (c) Microsoft Corporation.  All rights reserved.
*
* ==--==
* =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
*
* listener_construction_tests.cpp
*
* Tests cases for covering creating http_listeners in various ways.
*
* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
****/

#include "stdafx.h"

using namespace web::http;
using namespace web::http::experimental::listener;

using namespace tests::common::utilities;
using namespace tests::functional::http::utilities;

namespace tests { namespace functional { namespace http { namespace listener {

SUITE(listener_construction_tests)
{

TEST_FIXTURE(uri_address, default_constructor)
{
    // Test that the default ctor works.
    http_listener listener;

    VERIFY_IS_TRUE(listener.uri().is_empty());
    VERIFY_IS_TRUE(listener.open() != 0);
}

TEST_FIXTURE(uri_address, move_constructor_assignment, "Ignore:Linux", "TFS")
{
    http_listener listener = http_listener::create(m_uri);
    VERIFY_ARE_EQUAL(0, listener.open());
    test_http_client::scoped_client client(m_uri);
    test_http_client * p_client = client.client();

    // move constructor
    http_listener listener2 = std::move(listener);
    listener2.support(methods::PUT, [](http_request request)
    {
        http_asserts::assert_request_equals(request, U("PUT"), U("/"));
        request.reply(status_codes::OK);
    });
    VERIFY_ARE_EQUAL(0, p_client->request(U("PUT"), U("/")));
    p_client->next_response().then([](test_response *p_response)
    {
        http_asserts::assert_test_response_equals(p_response, status_codes::OK);
    }).wait();

    // move assignment
    listener = std::move(listener2);
    listener.support(methods::PUT, [](http_request request)
    {
        http_asserts::assert_request_equals(request, U("PUT"), U("/"));
        request.reply(status_codes::OK);
    });
    VERIFY_ARE_EQUAL(0, p_client->request(U("PUT"), U("/")));
    p_client->next_response().then([](test_response *p_response)
    {
        http_asserts::assert_test_response_equals(p_response, status_codes::OK);
    }).wait();
}

TEST_FIXTURE(uri_address, various_uris, "Ignore:Linux", "TFS")
{
    http_listener listener = http_listener::create(web::http::uri_builder(m_uri).append_path(U("path1")).to_uri());
    VERIFY_ARE_EQUAL(0, listener.open());
    test_http_client::scoped_client client(m_uri);
    test_http_client * p_client = client.client();

    // Path that matches exactly
    listener.support([](http_request request)
    {
        http_asserts::assert_request_equals(request, U("GET"), U(""));
        request.reply(status_codes::OK);
    });
    VERIFY_ARE_EQUAL(0, p_client->request(methods::GET, U("/path1/")));
    p_client->next_response().then([](test_response *p_response)
    {
        http_asserts::assert_test_response_equals(p_response, status_codes::OK);
    }).wait();

    // Path that matches but is more specific.
    listener.support([](http_request request)
    {
        http_asserts::assert_request_equals(request, U("GET"), U("/path2"));
        request.reply(status_codes::OK);
    });
    VERIFY_ARE_EQUAL(0, p_client->request(methods::GET, U("/path1/path2")));
    p_client->next_response().then([](test_response *p_response)
    {
        http_asserts::assert_test_response_equals(p_response, status_codes::OK);
    }).wait();

    // Try a request with a path that doesn't match.
    VERIFY_ARE_EQUAL(0, p_client->request(methods::GET, U("/path3/path2")));
    p_client->next_response().then([](test_response *p_response)
    {
        http_asserts::assert_test_response_equals(p_response, status_codes::NotFound);
    }).wait();
}

TEST_FIXTURE(uri_address, uri_error_cases)
{
    // non HTTP scheme
    VERIFY_THROWS(http_listener::create(U("ftp://localhost:456/")), std::invalid_argument);

    // empty HTTP host
    VERIFY_THROWS(http_listener::create(U("http://:456/")), std::invalid_argument);

    // try specifying a query
    VERIFY_THROWS(http_listener::create(U("http://localhost:45678/path?key1=value")), std::invalid_argument);

    // try specifing a fragment
    VERIFY_THROWS(http_listener::create(U("http://localhost:4563/path?key1=value#frag")), std::invalid_argument);
}

TEST_FIXTURE(uri_address, create_listener_get)
{
    http_listener listener = http_listener::create(m_uri, [](http_request request)
    {
        http_asserts::assert_request_equals(request, methods::GET, U("/"));
        request.reply(status_codes::OK);
    });
    
    VERIFY_ARE_EQUAL(0, listener.open());
    test_http_client::scoped_client client(m_uri);
    test_http_client * p_client = client.client();

    VERIFY_ARE_EQUAL(0, p_client->request(methods::GET, U("/")));
    p_client->next_response().then([](test_response *p_response)
    {
        http_asserts::assert_test_response_equals(p_response, status_codes::OK);
    }).wait();
    VERIFY_ARE_EQUAL(0, p_client->request(methods::PUT, U("/")));
    p_client->next_response().then([](test_response *p_response)
    {
        http_asserts::assert_test_response_equals(p_response, status_codes::MethodNotAllowed);
    }).wait();
}

TEST_FIXTURE(uri_address, create_listener_get_put)
{
    http_listener listener = http_listener::create(m_uri, [](http_request request)
    {
        http_asserts::assert_request_equals(request, methods::GET, U("/"));
        request.reply(status_codes::OK);
    }, [](http_request request)
    {
        http_asserts::assert_request_equals(request, methods::PUT, U("/"));
        request.reply(status_codes::OK);
    });
    
    VERIFY_ARE_EQUAL(0, listener.open());
    test_http_client::scoped_client client(m_uri);
    test_http_client * p_client = client.client();

    VERIFY_ARE_EQUAL(0, p_client->request(methods::GET, U("/")));
    p_client->next_response().then([](test_response *p_response)
    {
        http_asserts::assert_test_response_equals(p_response, status_codes::OK);
    }).wait();
    VERIFY_ARE_EQUAL(0, p_client->request(methods::PUT, U("/")));
    p_client->next_response().then([](test_response *p_response)
    {
        http_asserts::assert_test_response_equals(p_response, status_codes::OK);
    }).wait();
    VERIFY_ARE_EQUAL(0, p_client->request(methods::POST, U("/")));
    p_client->next_response().then([](test_response *p_response)
    {
        http_asserts::assert_test_response_equals(p_response, status_codes::MethodNotAllowed);
    }).wait();
}

TEST_FIXTURE(uri_address, create_listener_get_put_post)
{
    http_listener listener = http_listener::create(m_uri, [](http_request request)
    {
        http_asserts::assert_request_equals(request, methods::GET, U("/"));
        request.reply(status_codes::OK);
    }, [](http_request request)
    {
        http_asserts::assert_request_equals(request, methods::PUT, U("/"));
        request.reply(status_codes::OK);
    }, [](http_request request)
    {
        http_asserts::assert_request_equals(request, methods::POST, U("/"));
        request.reply(status_codes::OK);
    });
    
    VERIFY_ARE_EQUAL(0, listener.open());
    test_http_client::scoped_client client(m_uri);
    test_http_client * p_client = client.client();

    VERIFY_ARE_EQUAL(0, p_client->request(methods::GET, U("/")));
    p_client->next_response().then([](test_response *p_response)
    {
        http_asserts::assert_test_response_equals(p_response, status_codes::OK);
    }).wait();
    VERIFY_ARE_EQUAL(0, p_client->request(methods::PUT, U("/")));
    p_client->next_response().then([](test_response *p_response)
    {
        http_asserts::assert_test_response_equals(p_response, status_codes::OK);
    }).wait();
    VERIFY_ARE_EQUAL(0, p_client->request(methods::POST, U("/")));
    p_client->next_response().then([](test_response *p_response)
    {
        http_asserts::assert_test_response_equals(p_response, status_codes::OK);
    }).wait();
    VERIFY_ARE_EQUAL(0, p_client->request(methods::DEL, U("/")));
    p_client->next_response().then([](test_response *p_response)
    {
        http_asserts::assert_test_response_equals(p_response, status_codes::MethodNotAllowed);
    }).wait();
}

TEST_FIXTURE(uri_address, create_listener_get_put_post_delete)
{
    http_listener listener = http_listener::create(m_uri, [](http_request request)
    {
        http_asserts::assert_request_equals(request, methods::GET, U("/"));
        request.reply(status_codes::OK);
    }, [](http_request request)
    {
        http_asserts::assert_request_equals(request, methods::PUT, U("/"));
        request.reply(status_codes::OK);
    }, [](http_request request)
    {
        http_asserts::assert_request_equals(request, methods::POST, U("/"));
        request.reply(status_codes::OK);
    }, [](http_request request)
    {
        http_asserts::assert_request_equals(request, methods::DEL, U("/"));
        request.reply(status_codes::OK);
    });
    
    VERIFY_ARE_EQUAL(0, listener.open());
    test_http_client::scoped_client client(m_uri);
    test_http_client * p_client = client.client();

    VERIFY_ARE_EQUAL(0, p_client->request(methods::GET, U("/")));
    p_client->next_response().then([](test_response *p_response)
    {
        http_asserts::assert_test_response_equals(p_response, status_codes::OK);
    }).wait();
    VERIFY_ARE_EQUAL(0, p_client->request(methods::PUT, U("/")));
    p_client->next_response().then([](test_response *p_response)
    {
        http_asserts::assert_test_response_equals(p_response, status_codes::OK);
    }).wait();
    VERIFY_ARE_EQUAL(0, p_client->request(methods::POST, U("/")));
    p_client->next_response().then([](test_response *p_response)
    {
        http_asserts::assert_test_response_equals(p_response, status_codes::OK);
    }).wait();
    VERIFY_ARE_EQUAL(0, p_client->request(methods::DEL, U("/")));
    p_client->next_response().then([](test_response *p_response)
    {
        http_asserts::assert_test_response_equals(p_response, status_codes::OK);
    }).wait();
    VERIFY_ARE_EQUAL(0, p_client->request(methods::HEAD, U("/")));
    p_client->next_response().then([](test_response *p_response)
    {
        http_asserts::assert_test_response_equals(p_response, status_codes::MethodNotAllowed);
    }).wait();
}

}

}}}}
