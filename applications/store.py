from __future__ import print_function

import argparse

import odil

def add_subparser(subparsers):
    parser = subparsers.add_parser(
        "store", help="DICOM store (C-STORE)",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("host", help="Remote host address")
    parser.add_argument("port", type=int, help="Remote host port")
    parser.add_argument(
        "calling_ae_title", help="AE title of the calling application")
    parser.add_argument(
        "called_ae_title", help="AE title of the called application")
    parser.add_argument("filenames", nargs="+", help="File names")
    parser.set_defaults(function=store)
    return parser

def store(host, port, calling_ae_title, called_ae_title, filenames):
    transfer_syntaxes = [
        odil.registry.ExplicitVRLittleEndian,
        odil.registry.ImplicitVRLittleEndian,
    ]
    
    # Find all SOP classes to negotiate at association time. We don't need to
    # read the whole data set for this
    sop_classes = set()
    for filename in filenames:
        _, data_set = odil.read(
            filename, halt_condition=lambda tag: tag>odil.registry.SOPClassUID)
        sop_classes.update(data_set.as_string("SOPClassUID"))

    print(sop_classes)
    presentation_contexts = [
        odil.AssociationParameters.PresentationContext(
            2*i+1, sop_class, transfer_syntaxes, True, False)
        for i, sop_class in enumerate(sop_classes)
    ]
    
    # Create the association and the Store SCU
    association = odil.Association()
    association.set_peer_host(host)
    association.set_peer_port(port)
    association.update_parameters()\
        .set_calling_ae_title(calling_ae_title)\
        .set_called_ae_title(called_ae_title)\
        .set_presentation_contexts(presentation_contexts)
    association.associate()
    
    negotiated_parameters = association.get_negotiated_parameters()
    negotiated_pc = negotiated_parameters.get_presentation_contexts()
    for pc in negotiated_pc:
        print(pc.abstract_syntax, " ", pc.transfer_syntaxes[0])
    
    store = odil.StoreSCU(association)
    
    for filename in filenames:
        _, data_set = odil.read(filename)
        
        try:
            store.set_affected_sop_class(data_set)
            store.store(data_set)
        except Exception as e:
            print("Could not store {}: {}".format(filename, e))
    
    association.release()
