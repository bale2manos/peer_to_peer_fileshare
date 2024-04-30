import time, logging
from wsgiref.simple_server import make_server
from spyne import Application, ServiceBase, Integer, Unicode, rpc, String
from spyne.protocol.soap import Soap11
from spyne.server.wsgi import WsgiApplication
from datetime import datetime
class Current_TimeStamp(ServiceBase):
    @rpc(_returns=String)
    def take_timestamp(ctx):
        current_time = datetime.now()
        formatted_time = current_time.strftime("%d/%m/%Y %H:%M:%S")
        print("Fecha y hora actual formateadas:", formatted_time)
        return formatted_time

application = Application(services=[Current_TimeStamp], tns='http://tests.python-zeep.org/',
                          in_protocol=Soap11(validator='lxml'), out_protocol=Soap11())
application = WsgiApplication(application)
if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    logging.getLogger('spyne.protocol.xml').setLevel(logging.DEBUG)
    logging.info("listening to http://127.0.0.1:8000; wsdl is at: http://localhost:8000/?wsdl ")
    server = make_server('127.0.0.1', 8000, application)
    server.serve_forever()