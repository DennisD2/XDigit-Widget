


## The definition of a digit

```


    origin
   *----------------+	+ segment_margin      *---------+   +
   | 1------------2 |	+                     |   /\0   |   |
   | /            \ |                         | 5|  |1  |   |
   | \0          3/ |                         |  |  |   |   segment_height
   | 5------------4 |                         |  |  |   |   |
   +----------------+                         | 4|  |2  |   |
   +-+ segment_margin                         |   \/3   |   |
   ++ segment_delta                           +---------+   +
                                              +---------+ segment_width 
```

Segment numbers:
``` 
        --1--
       |     |
       2 -9- 3
       |--4--|
       5 -*- 6 (*=10)
       |     |
        --7--  -8-
```

 The resource XtNvalue controls the digit to be displayed. The value
 corresponds to the displayed number, except the following values:
 * MINUS_VALUE : minus (segment 4)
 * DECPOINT_VALUE : decimal point only (segment 8)
 * DOUBLEPOINT_VALUE : double point (segments 9,10)

