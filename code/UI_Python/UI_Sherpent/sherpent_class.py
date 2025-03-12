from module_class import Modules
import gc

class Sherpent:
    def __init__(self):
        self.modules_list = []
        self.nbr_modules = 0

    def add_module(self):
        """Add module to the list"""

        module = Modules(len(self.modules_list))
        if isinstance(module, Modules):
            self.modules_list.append(module)
            self.nbr_modules = len(self.modules_list)
        else:
            print("--> Erreur Sherpent: L'objet ajouté n'est pas un module valide.")

    def delete_module(self, index):
        """Supprime un module et force sa destruction."""
        if 0 <= index < len(self.modules_list):
            deleted_module = self.modules_list[index]  # Récupère le module pour vérification
            del self.modules_list[index]  # Supprime le module de la liste

            # Vérifie s'il reste des références et force la destruction
            if not any(deleted_module is m for m in self.modules_list):
                del deleted_module
                gc.collect()  # Force le garbage collector

            self.nbr_modules = len(self.modules_list)

            print(f"--> Module à l'index {index} supprimé et mémoire libérée.")
        else:
            print("--> Erreur Sherpent: Index invalide.")

    def get_modules(self):
        return self.modules_list

    def get_nbr_modules(self):
        return self.nbr_modules