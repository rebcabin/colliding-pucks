#  ___             _   _               _   ___     _       _ _   _
# | __|  _ _ _  __| |_(_)___ _ _  __ _| | | _ \_ _(_)_ __ (_) |_(_)_ _____ ___
# | _| || | ' \/ _|  _| / _ \ ' \/ _` | | |  _/ '_| | '  \| |  _| \ V / -_|_-<
# |_| \_,_|_||_\__|\__|_\___/_||_\__,_|_| |_| |_| |_|_|_|_|_|\__|_|\_/\___/__/


def pairwise(ls, fn):
    result = []
    for i in range(len(ls) - 1):
        temp = fn(ls[i], ls[i + 1])
        result.append(temp)
    return result


def pairwise_toroidal(ls, fn):
    return pairwise(ls + [ls[0]], fn)


