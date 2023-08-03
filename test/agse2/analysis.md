# Analysis of data issue.

1. Install draw.io integration for vscode
2. Open agse-decomposition.drawio

## DATA SOURCE
===

```
DECLARE a INTEGER, b INTEGER, c INTEGER STREAM core, 1 FILE 'datafile.txt'
```
```
10 11 12
13 14 15
16 17 18
19 20 21
22 23 24
25 26 27
28 29 30
```

---------------------------------------------------------------------------
## serial1 - __WRONG__
===
```
SELECT * STREAM serial1 FROM core@(1,1)
```
```
{ core_0:11 }
{ core_0:17 }
{ core_0:20 }
{ core_0:20 }
{ core_0:20 }
{ core_0:23 }
```
### Analysis:

> should look like:
```
{ core_0:11 }
{ core_0:12 }
{ core_0:13 }
{ core_0:14 }
{ core_0:15 }
{ core_0:16 }
```

---------------------------------------------------------------------------

## serial2
>> __WRONG !__

```
SELECT * STREAM serial2 FROM core@(2,2)
```
> Actual:
```
{ core_0:24 core_1:19 }
{ core_0:21 core_1:16 }
{ core_0:24 core_1:19 }
{ core_0:24 core_1:19 }
{ core_0:27 core_1:22 }
```
> This should look like:
```
{ core_0:24 core_1:25 }
{ core_0:26 core_1:27 }
{ core_0:28 core_1:29 }
{ core_0:30 core_1:10 }
{ core_0:11 core_1:12 }
```

---------------------------------------------------------------------------
### serial 3 

>> __WRONG !__

```
SELECT * STREAM serial3 FROM core@(1,2)
```
> Actual:
```
{ core_0:14 core_1:15 }
{ core_0:20 core_1:21 }
{ core_0:20 core_1:21 }
{ core_0:20 core_1:21 }
{ core_0:20 core_1:21 }
{ core_0:23 core_1:24 }
```
> This should look like:
```
{ core_0:14 core_1:15 }
{ core_0:15 core_1:16 }
{ core_0:16 core_1:17 }
{ core_0:17 core_1:18 }
{ core_0:18 core_1:19 }
{ core_0:19 core_1:20 }
```
---------------------------------------------------------------------------

## serial4 - __OK__

```
SELECT * STREAM serial4 FROM core@(3,2)
```
```
{ core_0:16 core_1:17 }
{ core_0:19 core_1:20 }
{ core_0:22 core_1:23 }
{ core_0:25 core_1:26 }
record out of range - list command
record out of range - list command
```

---------------------------------------------------------------------------
## serial5 - __OK__

```
SELECT * STREAM serial5 FROM core@(3,3)
```
```
{ core_0:16 core_1:17 core_2:18 }
{ core_0:19 core_1:20 core_2:21 }
{ core_0:22 core_1:23 core_2:24 }
{ core_0:25 core_1:26 core_2:27 }
record out of range - list command
record out of range - list command
```
---------------------------------------------------------------------------
## serial6 - __OK__

```
SELECT * STREAM serial6 FROM core@(3,-3)
```
```
{ core_0:18 core_1:17 core_2:16 }
{ core_0:21 core_1:20 core_2:19 }
{ core_0:24 core_1:23 core_2:22 }
{ core_0:27 core_1:26 core_2:25 }
record out of range - list command
record out of range - list command
```
---------------------------------------------------------------------------
## serial7
>> __WRONG !__

```
SELECT * STREAM serial7 FROM core@(2,3)
```
> Actual
```
{ core_0:30 core_1:25 core_2:26 }
{ core_0:21 core_1:16 core_2:17 }
{ core_0:24 core_1:19 core_2:20 }
{ core_0:24 core_1:19 core_2:20 }
{ core_0:27 core_1:22 core_2:23 }
{ core_0:30 core_1:25 core_2:26 }
```

how - todo.
===

## serial8
>> __WRONG !__

```
SELECT * STREAM serial8 FROM core@(2,4)
```
```
{ core_0:15 core_1:10 core_2:11 core_3:12 }
{ core_0:21 core_1:16 core_2:17 core_3:18 }
{ core_0:24 core_1:19 core_2:20 core_3:21 }
{ core_0:24 core_1:19 core_2:20 core_3:21 }
{ core_0:27 core_1:22 core_2:23 core_3:24 }
{ core_0:30 core_1:25 core_2:26 core_3:27 }
```
how - todo.
===