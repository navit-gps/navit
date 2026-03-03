Embedding
=========

You can embed navit in two ways: you can make navit embed itself by
setting NAVIT_XID.

Here is a Qt sample to embed navit's window into your own window:

.. code:: cpp

    container = new QX11EmbedContainer(this);
    QString wId = QString::number(container->winId());
    setenv("NAVIT_XID", wId.toAscii(), 1);
    process = new QProcess(container);
    process->start("navit");

Navit can also get "pulled" by using a corresponding function of your
project (e.g. void QX11EmbedContainer::embedClient(WId id) for Qt 4.4).

Here is a gtk sample to embed navit's window into you own window:
http://libgarmin.sf.net/zfe.tgz

See also
--------

-  `Dbus <Dbus>`__
