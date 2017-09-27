# CENO2 architecture design, September 2017

This overview contains a description of the different components that will make up CENO2, including the client library, the injector infrastructure, and supporting infrastructure.

## Shared components

### Decentralized static content caches

The CENO2 project as a whole contains several implementations of the _content cache_ abstract interface. This collection of content cache implementations is used at several places in the overall CENO2 project.

A _content cache_ implementation is a system that can access stored copies of web resources by URL, and share such copies with the world. A content cache is *not* responsible for trying to acquire resources from origin servers in order to share them; rather, given a copy of a resource, it can seed that resource. Similarly, a content cache is not responsible for determining what resources to cache.

Theoretically, a content cache should be able to store arbitrary byte streams. In practice, we probably want to use it to store combinations of HTTP headers / content / caching directives, rather than bare contents. This makes it possible to cache a resource with all headers attached; additionally, it makes it possible for a content cache to determine when a cache entry has expired.

A content cache implements the following abstract interface:

* Try to retrieve the resource identified by $URL. This is an asynchronous operation. On successful completion, this returns a tuple of (headers, content, caching directives).
* Given a resource already on disk somewhere consisting of (headers, content, caching directives), and identified by $URL, start seeding this resource.
* Stop seeding resource $URL.
* Estimate the network availability of the resource identified by $URL. This asynchronously returns information on the amount of copies of the resource available in the network. This operation returns an estimate, not exact numbers, for the purposes of determining how valuable extra seeds for this resource would be.

A content cache implementation may be directed by a resource scheduler component (below), setting restraints on the usage of resources such as bandwidth and energy.

The decision on what resources to cache, and for how long, based on considerations such as available disk space and availability of resources, is made by the cache manager component (below). The cache manager directs content cache implementations to start and stop seeding resources as it decides to add and remove resources to the disk store. Cached content is generally seeded on all available content cache implementations, and clients can try to access data from whichever content cache systems they can successfully access.

Potential implementations include:

* An IPFS-based cache;
* A bittorrent-based cache;
* A torrentless p2p cache using the bittorrent DHT for peer discovery (Clostra).


### Hidden decentralized services

CENO2 contains several implementations of the _hidden service_ abstract interface. This general system is used whenever a potentially-censored user needs to connect to an arbitrary node willing to perform a service.

A _hidden service_ is a named set of nodes that are each hosting a networked service of some sort, accessible by some TCP-like protocol. Hidden services are designed in such a way that clients of a service aim to connect to one or more *arbitrary* nodes hosting the service, akin to a load balancer. From the point of view of a node hosting a service, a hidden service behaves just like a TCP server; from the point of view of a client, hidden services have the operation "set up a TCP-like connection with some node in the service called $name".

For pluggable-modules reasons, CENO2 will contain several such implementations. Each hidden service can generally be accessed by all available hidden service implementations, and clients can use whichever implementations are not blocked.

A hidden service implementation implements the following abstract interface, modelled after traditional TCP services:

* Participate in hosting a hidden service identified by $name. This is an asynchronous operation similar to setting up a TCP server socket; it asynchronously yields incoming connections represented by some implementation of a stream socket.
* Make a connection to any one [should we support multiple?] node in the hidden service identified by $name. This is an asynchronous operation similar to connecting a TCP client socket; if successful, it asynchronously returns a stream socket.

Besides implementing these two operations, any implementation of a hidden service may also as a background service altruistically participate in keeping the hidden service implementation operational. 

Potential implementations of the hidden service system include:

* dCDN injector-proxy system;
* I2P;
* A DHT routing algorithm.



## Injector infrastructure

The primary support infrastructure behind CENO2, ran (at least initially) by eQualit.ie, is a collection of _injector servers_, for which we need a better name. Conceptually, injector servers are open web proxies, accessible as a CENO2 hidden service.

eQualit.ie runs a collection of what are basically open proxies, which can be accessed through the hidden-service mechanisms implemented in CENO2. The proxies will accept all HTTP requests, and possibly cache the results if eligible. Together, these proxies form a hidden decentralized service, allowing a client to make a connection to some unspecified CENO2 proxy in a censorship-resistant way.

The injector hidden service forms the method by which users can access dynamic webpages. However, its usage is not limited to dynamic webpages; instead, the intent is that the injector service is the first line of access for ALL webpage requests (when an origin server is blocked, at least). This avoids the problem where a client needs to determine in advance whether a given request is dynamic or not. Instead, clients can and do use the injector service for all page requests; injector servers can decide that the result of a request is cacheable, and act accordingly, AFTER a request has been completed.

After handling a proxied HTTP request, injector servers determine whether the result is cacheable. If so, they insert the resulting resource into the different decentralized content caches, after signing the result and determining the detailed limitations of cache eligibility.



## Client library

The _client library_ is the component of CENO2 that runs as part of CENO2-protected applications. It provides an API for accessing a web resource through the CENO2 mechanisms, and implements platform-specific techniques to make sure that all web requests are wrapped by this API. Additionally, the client library altruistically participates in running the CENO2 network, seeding cached resources and serving as a node in several hidden service implementations.



### Request dispatcher

The request dispatcher is the API for accessing web resources through the CENO2 framework. It routes web requests to the appropriate content access mechanisms, according to network conditions. It consists of a single API for accessing a web resource using the straightforward interface.

The exact policy applied by the request dispatcher remains open to debate, but the proposed workflow for answering a web resource request is as follows:

TODO TODO TODO

* Try to connect to the origin server directly, and keep close track of whether or not the request is accessible. (*Origin server client*)
* If that fails, try to connect to an injector, using the different hidden service implementations, and request the resource there.
* If that fails, try to fetch the resource from the static cache.



### Origin server client

TODO TODO TODO

This component is useful in client configurations where the origin server is contacted before using other CENO2 mechanisms.  It is designed as a black box HTTP implementation to be used by the request dispatcher when communicating with the origin server. It performs a series of checks along the way, for instance:

* DNS replies
* DNS fingerprinting
* HTTP server fingerprinting
* Block page signature detection on the output
* Area-based blacklist checking

As a result, it can return extra error statuses like "network filtering error", which can be used by the request dispatcher to further decide how to proceed with the request.

TODO TODO TODO


### Cache manager

The cache manager is responsible for deciding what resources to store in the local disk cache, subject to a finite amount of usable disk space. Based on judgments of the available disk space, and user settings, its function is to decide what cached resources to spend this disk space on. It determines when to add resources to the cache, and what resources to expunge to make room.

In determining this, the cache manager takes the following considerations into account:

* Available disk space
* Availability of resources in the static cache (scarce resources are higher-priority to cache than common resources)
* Freshness of resources (recent resources are higher priority than dated ones)
* Anything else?

Besides choosing whether or not to cache a resource on disk, the cache manager may also decide to make resources more available by utilizing the altruistic seeder service. See below.


### Resource scheduler

CENO2 will run on many machines on which resources are limited. Besides the limitations of disk space, most machines will have a limited amount of bandwidth that can be spent on such things as seeding resources or otherwise serving the network; moreover, many machines will be on battery power, which CENO2 should use sparingly.

It is the job of the resource scheduler to determine when and how much resources to spend at operations that serve the network. How to determine this is for now an open question. An altruism budget that is a configurable percentage of total nonaltruistic bandwidth used by CENO2?


### Altruistic seeder service

The CENO2 client library [and possibly some other components as well? Could this be a standalone tool?] participate in a hidden service known as the *altruistic seeder service*. Nodes serving as part of the altruistic seeder service will, on request, download a copy of a given resource from the static cache, and then seed that resource for some time.

The altruistic seeder service consist of a network service offering a single operation: "please increase the availability of the resource $URL". This is a best-effort operation; nodes hosting the altuistic seeder service use the cache manager component to determine whether or not to honour the request. If the cache manager component at a given altruistic seeder node decides that hosting that particular resource is not an efficient use of resources, the altruistic seeder server will deny the request.

Any node not satisfied by the degree of availability of a given resource can ask the altruistic seeder service to improve this availability. By contacting multiple altruistic seeders, it can increase the resource availability until it has become satisfactory, or give up if too many servers deny the request.


### Web proxy

The client library will contain platform-specific measures that wrap all web resource access in the CENO2 API. For Android, this can be nicely implemented by a HTTP proxy, set on a per-app basis via a unix environment variable.

This takes the form of a HTTP proxy that serves as a thin wrapper, routing all incoming requests to the CENO2 request dispatcher.




## Support services




