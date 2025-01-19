---
layout: single
permalink: /different/
title: "Different Time Series DB"
header:
  overlay_color: "#000"
  overlay_filter: "0.5"
  overlay_image: /assets/images/IMG_0758.JPG
  caption: "Photo credit: Tomasz Widera"
excerpt: "RetractorDB is the open source time series database"
toc: true
---
contents of this page is _work in progress_.
Page under construction.

# RetractorDB - different aproach to time series processing.

## Abstract

This page is a actual report describing work on following problem: How different RetractorDB compare to other solutions. First, we are trying to find out what we understand by other solutions and what we are will compare with. Then this page is showing how simple examples of time series processing on identified system and how actually we can do it in RetractorDB. Then we are presenting tasks that are hard or impossible to write and process in competitors. Then we will try something opposite - write tasks that are hard or impossible to write in RetractorDB and how easy we can do it in other solutions. At the summary section we will show how to connect to other time series database systems and get benefits for different solutions. How to cooperate and get benefits of different approaches.


## Introduction

First, we need to find what time series databases are close to our RetractorDB.
In a related paper [Data Series Management: Fulfilling the Need for Big Sequence Analytics][KT-2018] we can find reference to [Db-engines page][DB-RANK] with ranking categories. When we narrow our scope to time series dbms we can find actual time series leaders. This page is updated continuously. Method of so called ‘ranking’ is based on mentions in web – methodology is explained on this page.  There is significant factor in this ranking score call “include secondary database model”. If we turn this checkbox on we can see systems that time series processing functionality is second or third potential functionality, but theirs adaptation are is mainly in NoSQL task rather than time series processing problems.

Based on db-engines rank page I’ve chose InfluxDB, Prometheus and OpenTSDB. Intentionally skipping Kdb+ cause of strict licensing and close source model. Additionally, Kdb+ license show that we are not able to make any benchmark or report unless end user receives express, prior written consent.

* InfluxDB is on open source MIT license.
* Prometheus is on Apache 2.0 license.
* OpenTSDB is on LGPL

That’s license friendly products that can compare to and promote for community.

## Time series processing tasks

There are various group of typical time series processing tasks. Most simples are: show me histogram of changes or show average value of incoming measurement. That's basics what we can ask typical time series database system.
Things became more complicated when someone asks - how many data you want to use? If you accidentally touch infinity - you can doubt what was really means "simple".

First we need to realize that RetractorDB works currently only on __regular__ time series. In case of general Time Series Databases - they support both regular and irregular time series.

### Downsampling

The idea of downsamplig with time series database was well descibed in one of [InfluxDB presentations][YT-FL-DOWNSAMPL]. According to [OpenTSDB Documentation][OPENTSDB-DOWNSAMPL] downsampling (or in signal processing, decimation) is the process of reducing the sampling rate, or resolution, of data.

[KT-2018]:http://helios.mi.parisdescartes.fr/~themisp/publications/icde18-sms.pdf
[DB-RANK]:https://db-engines.com/en/ranking/time+series+dbms
[YT-FL-DOWNSAMPL]:https://youtu.be/j3x0TohyGJY
[OPENTSDB-DOWNSAMPL]:http://opentsdb.net/docs/build/html/user_guide/query/downsampling.html
[OPENTSDB-AGREGATORS]:http://opentsdb.net/docs/build/html/user_guide/query/aggregators.html
[PROMETHEUS-AGREGATORS]:https://prometheus.io/docs/prometheus/latest/querying/operators/#aggregation-operators
