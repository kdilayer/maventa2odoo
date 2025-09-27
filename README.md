**Maventa2odoo**

A tool that integrates Maventa invoicing with Odoo invoicing.

https://maventa.com/

https://www.odoo.com/ (tested with odoo 18)

maventa2odoo is a command line tool. 
The tool is intended to be run periodically (e.g. crontab) or it can be used as a library.

Features: 
- send e-invoices (finvoice 3.0) from odoo
- receive e-invoices (finvoice xxx) to odoo
- supports attachments in both directions

Internally the tool will:
- pull invoices from odoo and send them to maventa
- pull (incoiming) invoices from maventa and add them to odoo
- attachments are supported
- internally uses finvoice xml version 3.0

Run as command line:
maventa2odoo -c maventa2odoo.conf

**Configuration**
<pre>
{
    "profiles": [
        {
            "name": "Comapny name One",                            => name of profile usually company name, only used for debugging
            "maventa_client_id": "User API key",                   => get this from maventa or ask
            "maventa_client_secret": "Company UUID key",           => get this from maventa or ask
            "maventa_vendor_api_key": "Vendor API key",            => get this from maventa or ask

            "odoo_url": "https://xxx-url-db-237848.dev.odoo.com/", => this your odoo server url
            "odoo_db": "db-237848",                                => odoo database id
            "odoo_username": "my_user@mydomain.com",               => odoo user name 
            "odoo_api_key": "odoo api key",                        => odoo api key
            "odoo_company_id": 2                                   => odoo company id 
        },
        {
         ....                                                      => other profiles in case you have many companies
        }
    ]
    
}
</pre>
