**Maventa2odoo**

A tool that integrates Maventa invoicing with Odoo invoicing.

https://maventa.com/
https://www.odoo.com/

maventa2odoo is a command line tool. 
The tool intended to be run periodically (e.g. crontab).

Features: 
- send e-invoices from odoo
- receive e-invoices to odoo
- supports attachments in both directions

Internally the tool will:
- pull invoices from odoo and send the to maventa
- pull (incoiming) invoices from maventa and add them to odoo
- attachments are supported
- internally uses finvoice xml version 3.0

Run on command line:
maventa2odoo -c maventa2odoo.conf

**Configuration**
