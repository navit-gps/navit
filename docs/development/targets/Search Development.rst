.. _search_development:

Search Development
==================

How to use the search API of navit

First get a handle for searching, specifying the mapset in which the
search should be performed.

.. code:: c

    search_list=search_list_new(struct mapset *ms);

Then specify for what to search. The following attributes are supported
by the search api, but not all might be possible with all maps
attr_country_id (ISO numeric country ID),attr_country_iso2 (ISO 2 letter
country id),attr_country_iso3 (ISO 3 letter country id) attr_country_car
(Car country sign),attr_country_name (country name in the set language),
attr_town_postal (Postal Code of a Town), attr_town_name (Name of a
Town), attr_street_name (Name of a street) You can set partial=1 to
search for partial matches

.. code:: c

    partial=0;
    attr.type=attr_country_iso2;
    attr.u.str="DE";
    search_list_search(search_list, &attr, partial);

Then you can query the result

.. code:: c

    result=search_list_get_result(search_list);

If the result is NULL, the search is done and there are no more results

After the search for a country you can continue to search for a town

.. code:: c

    attr.type=attr_town_name;
    attr.u.str="München";
    search_list_search(search_list, &attr, partial);

and then for a street

.. code:: c

    attr.type=attr_street_name;
    attr.u.str="Marienplatz";
    search_list_search(search_list, &attr, partial);

If you get several results in search_list_get_result and want to limit
them for further searches, you can call

.. code:: c

    search_list_select(search_list, attr.type, result->id, 1);

to pick a particular result.
