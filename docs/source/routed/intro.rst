.. _common:

Introduction
============

The binary `osrm-routed` is a basic HTTP/1.0 server that supports a 'keep-alive'
extension. Persistent connections are limited to 512 requests per connection and allow
no more than 5 seconds between requests.

Conventions
-----------

A parameter described as taking an 'array<element>' expects you to provide a
semicolon-separated list of elements. An empty list entry means the default value for
element, if there is one, eg.: in the array `1;2;;4;5` the third value will be set to the
default value.

A parameter described as taking a 'keyword' expects one of a fixed set of words.

The default value for a parameter, if any, is typeset in bold: eg.: :default:`false`,
`true`.


Parameters Common to All Services
---------------------------------

.. http:get:: /(service)/(version)/(profile)/(coordinates)[.(format)]

   Common request structure

   .. dropdown:: Example Requests

      .. code:: bash

         curl 'http://router.project-osrm.org/GET /route/v1/driving/13.388860,52.517037;13.397634,52.529407;13.428555,52.523219?overview=false'
         curl 'http://router.project-osrm.org/GET /route/v1/driving/13.388860,52.517037;13.397634,52.529407?exclude=motorway'
         curl 'http://router.project-osrm.org/GET /route/v1/driving/polyline(ofp_Ik_vpAilAyu@te@g`E)?overview=false'

   .. _common_options:

   URL parameters common to all services:

   :param string service:
      One of the following values: :ref:`route <route>`, :ref:`nearest <nearest>`,
      :ref:`table <table>`, :ref:`match <match>`, :ref:`trip <trip>`, or :ref:`tile
      <tile>`

   :param string version:
      Version of the protocol implemented by the service. `v1` for all OSRM 5.x and 6.x
      installations

   .. include:: common_parameters.rst

   Query parameters common to all services:

   .. include:: common_queries.rst

   Responses common to all services:

   .. include:: common_responses.rst

   Status codes common to all services:

   .. include:: common_statuscodes.rst

   Example of common response codes:

   .. code:: json

      {
      "code": "Ok",
      "message": "Everything worked",
      "data_version": "2017-11-17T21:43:02Z"
      }
