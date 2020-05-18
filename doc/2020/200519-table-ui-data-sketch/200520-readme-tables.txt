drafting table data format
notes 17.8.2020/pekka

Table JSON contains data value and metadata. It doesn't contain visual information, 
like fonts, colors, etc. That is up to user interface.

Basic sturcture is that:
- table has name, which is often the database table name. It identifies the table, contains select criterias and pageing of long tables.
- The table "columns" contains only column names and column specific metadata.
- The table "rows" array holds actual data for each column. The items here are placed by column name to
  specific column. (Binary JPEG compression makes this inexpensive).

Getting metadata for item
Metadata for item, row, column, and table is merged to get metadata attributes to draw an item.
If same metadata attribute is listed in several places, "item's" metadata has highest priority,
then row metadata, column metadata and table metadata has lowest priority.

Selecting data
When user interface wants to select data from a table, it uses the same JSON format, but severely stripped down.
Selection criterias used are returned as given.

Updates, inserts and removes.
Similarly to selecting data, requests to modify the table are "almost" subsets to table JSON. 
For update and remove the "select" informs which row or rows to modify.

Trend specific
To draw a trend line we need "extra" values before and after time period being plotted. This is needed to keep 
trend lines from the first and last point to be drawn going to right direction. The extra data may also be used
to average different time intervals overlapping displayed limits.

Displaying large data set embedded as table column
For example a high large bitmap image or large data matrix cannot be shown as table element. Also transferring
these elements for every displayed row can make data transfer prohitively large. 
No data in items must be selected separately

Column groups
Often tables are so wide that the it is not convinient to scroll the table left and right.
Column groups  can be used to group To keep tRows can be grouped with "group" name into sets of parameters.
It can be used also to group set of rows beloning to same set together.
Special column group "header" is used to lock columns as part row header, which is not scrolled left and right with data.

Row groups
Rows can be grouped with "group" name into sets of parameters.
It can be used also to group set of rows beloning to same set together.

Table uses
The table "use" spefifies general use of table. User interface may draw the table differently based
on table use. For example it is customary to draw parameter settings differently than time based
measurement data. Uses are
- "timebase" Table contains measurement history over time. These tables may be long (years of measurement
data and have hundreads of columns), thus paging, averaging and column grouping are useful. It can
be assumed that table has "timestamp" column.
- "parameters" Table contains parameter settings. Parameter settings table typically has two columns,
parameter name and parameter value. (do we add column for unit, or should UI do it automatic?). 
Parameter table is often divided into paragraps (here row groups), each having it's own table.
indicated as group header "group"
- "table" Table contains parameter table or other table like it. The columns can be anything
and all rows can be displayed as one page (with scroll bar if needed). No paging or averaging
is typically needed.

Dynamic updates
If underlaying data in server end changes, and the change effects the selected data in table,
the server will send partial table JSON to indicate change. If this is remove or update, the 
"select" items indicate which rows are to be updated or removed.

Versioning
If one receives JSON with unknown tag, it should be quietly ignored. For tags not in input JSON,
best quess should be made (should be decided what are the best quessess for each attribute).

Loose typing
Data typing for JSON tags is loose. If receiver needs string and data or attribute is something else,
data is converted to string. Similarly to other direction, if number is needed (like for trend line),
string data is converted to one.
