#include "assert.h"

#define SQLITE_TOOLS_USE_JSON_TOOLS
#include <sqlite3/sqlite3.h>
#include <json_tools/json_tools.h>
#include <sqlite_tools.h>
#include "recipes-db.h"

const char jsonDatabase[] = R"json(
{
    "recipes": [
        {
            "id": 1,
            "name": "Sausages with Mashed Potatoes",
            "cooking_time": 10,
            "portions": 2,
            "portions_unit": "portions",
            "description": "The good old one that never fails",
            "favorite": 0
        },
        {
            "id": 2,
            "name": "Cauliflower Bonanzá",
            "cooking_time": 25,
            "portions": 4,
            "portions_unit": "bowls",
            "description": "Gets your belly rumbling!",
            "favorite": null
        },
        {
            "id": 3,
            "name": "Peter's Speciality",
            "cooking_time": 20,
            "portions": 4,
            "portions_unit": "portions",
            "description": null,
            "favorite": 1
        },
        {
            "id": 4,
            "name": "The Stew",
            "cooking_time": 23,
            "portions": 1,
            "portions_unit": "huge cauldron",
            "description": "It's stew",
            "favorite": 0
        },
        {
            "id": 5,
            "name": "Spaghetti Bolognese",
            "cooking_time": 30,
            "portions": 4,
            "portions_unit": "big plates",
            "description": "Classic!",
            "favorite": 1
        }
    ],
    "ingredient_in_recipe": [
        {
          "recipe_id": 1,
          "ingredient_id": 9
        },
        {
          "recipe_id": 1,
          "ingredient_id": 10
        },
        {
          "recipe_id": 2,
          "ingredient_id": 6
        },
        {
          "recipe_id": 3,
          "ingredient_id": 1
        },
        {
          "recipe_id": 3,
          "ingredient_id": 2
        },
        {
          "recipe_id": 3,
          "ingredient_id": 3
        },
        {
          "recipe_id": 3,
          "ingredient_id": 4
        },
        {
          "recipe_id": 3,
          "ingredient_id": 8
        },
        {
          "recipe_id": 4,
          "ingredient_id": 1
        },
        {
          "recipe_id": 4,
          "ingredient_id": 8
        },
        {
          "recipe_id": 4,
          "ingredient_id": 9
        },
        {
          "recipe_id": 5,
          "ingredient_id": 4
        },
        {
          "recipe_id": 5,
          "ingredient_id": 5
        }
    ],
    "ingredients": [
        {
            "id": 1,
            "name": "Carrots",
            "description": "They're orange",
            "vegetarian": 1,
            "vegan": 1
        },
        {
            "id": 2,
            "name": "Cheese",
            "description": "Any kind of cheese will do",
            "vegetarian": 1,
            "vegan": 0
        },
        {
            "id": 3,
            "name": "Peanut Butter",
            "description": "100 % Peanuts",
            "vegetarian": 1,
            "vegan": 1
        },
        {
            "id": 4,
            "name": "Meatballs",
            "description": null,
            "vegetarian": 0,
            "vegan": 0
        },
        {
            "id": 5,
            "name": "Spaghetti",
            "description": "Mom's Spaghetti",
            "vegetarian": 1,
            "vegan": 1
        },
        {
            "id": 6,
            "name": "Cauliflower",
            "description": null,
            "vegetarian": 1,
            "vegan": 1
        },
        {
            "id": 7,
            "name": "Beef",
            "description": "Moooo",
            "vegetarian": 0,
            "vegan": 0
        },
        {
            "id": 8,
            "name": "Spices",
            "description": "Spices will do for any recipe",
            "vegetarian": 1,
            "vegan": 1
        },
        {
            "id": 9,
            "name": "Potatoes",
            "description": null,
            "vegetarian": 1,
            "vegan": 1
        },
        {
            "id": 10,
            "name": "Sausages",
            "description": "We need these as well",
            "vegetarian": 0,
            "vegan": 0
        }
    ],
    "allergen_in_ingredient": [
        {
           "ingredient_id": 2,
           "allergen_id": 1
        },
        {
           "ingredient_id": 3,
           "allergen_id": 4
        },
        {
           "ingredient_id": 5,
           "allergen_id": 2
        }
    ],
    "allergens": [
        {
            "id": 1,
            "name": "Milk",
            "description": null
        },
        {
          "id": 2,
          "name": "Gluten",
          "description": null
        },
        {
          "id": 3,
          "name": "Nuts",
          "description": "All kinds of nuts."
        },
        {
          "id": 4,
          "name": "Peanuts",
          "description": "Deadly!"
        }
    ]
}
)json";

int main()
{
    // 0 Parse JSON data from string into dataContainer
    recipes_db::DatabaseContainer dbContainer;
    JT::ParseContext context(jsonDatabase);
    context.parseTo(dbContainer);
    assert(context.error == JT::Error::NoError);

    char* errMsg;
    int result;

    // 1. Drop all existing tables to clear the data
    result = SQLT::dropAllTables<recipes_db>(&errMsg);
    SQLT_ASSERT(result == SQLITE_OK);

    // 2. Create all tables
    result = SQLT::createAllTables<recipes_db>(&errMsg);
    SQLT_ASSERT(result == SQLITE_OK);

    // 3. Insert all data
    result = SQLT::insert<recipes_db>(dbContainer.recipes);                             assert(result == SQLITE_OK);
    result = SQLT::insert<recipes_db>(dbContainer.ingredient_in_recipe);                assert(result == SQLITE_OK);
    result = SQLT::insert<recipes_db>(dbContainer.ingredients);                         assert(result == SQLITE_OK);
    result = SQLT::insert<recipes_db>(dbContainer.allergen_in_ingredient);              assert(result == SQLITE_OK);
    result = SQLT::insert<recipes_db>(dbContainer.allergens);                           assert(result == SQLITE_OK);

    // 4. Select all data
    recipes_db::DatabaseContainer selectedContainer;
    result = SQLT::selectAll<recipes_db>(&selectedContainer.recipes);                   assert(result == SQLITE_OK);
    result = SQLT::selectAll<recipes_db>(&selectedContainer.ingredient_in_recipe);      assert(result == SQLITE_OK);
    result = SQLT::selectAll<recipes_db>(&selectedContainer.ingredients);               assert(result == SQLITE_OK);
    result = SQLT::selectAll<recipes_db>(&selectedContainer.allergen_in_ingredient);    assert(result == SQLITE_OK);
    result = SQLT::selectAll<recipes_db>(&selectedContainer.allergens);                 assert(result == SQLITE_OK);

    // 5. Verify that all data is selected
    assert(dbContainer.recipes.size()                 ==  selectedContainer.recipes.size());
    assert(dbContainer.ingredient_in_recipe.size()    ==  selectedContainer.ingredient_in_recipe.size());
    assert(dbContainer.ingredients.size()             ==  selectedContainer.ingredients.size());
    assert(dbContainer.allergen_in_ingredient.size()  ==  selectedContainer.allergen_in_ingredient.size());
    assert(dbContainer.allergens.size()               ==  selectedContainer.allergens.size());

    // 6. Select single rows only and verify some of the data.
    std::vector<decltype(recipes_db::recipes::id)> recipeIds;
    result = SQLT::select<recipes_db, recipes_db::recipes>(&recipes_db::recipes::id, &recipeIds, 5);
    SQLT_ASSERT(result == SQLITE_OK);
    SQLT_ASSERT(recipeIds.size() == 5);
    SQLT_ASSERT(recipeIds[0] == 1);
    SQLT_ASSERT(recipeIds[4] == 5);

    std::vector<decltype(recipes_db::recipes::name)> recipeNames;
    result = SQLT::select<recipes_db, recipes_db::recipes>(&recipes_db::recipes::name, &recipeNames, 5);
    SQLT_ASSERT(result == SQLITE_OK);
    SQLT_ASSERT(recipeNames.size() == 5);
    SQLT_ASSERT(recipeNames[1] == "Cauliflower Bonanzá");
    SQLT_ASSERT(recipeNames[2] == "Peter's Speciality");
    SQLT_ASSERT(recipeNames[3] == "The Stew");

    std::vector<decltype(recipes_db::recipes::portions)> recipePortions;
    result = SQLT::select<recipes_db, recipes_db::recipes>(&recipes_db::recipes::portions, &recipePortions, 5);
    SQLT_ASSERT(result == SQLITE_OK);
    SQLT_ASSERT(recipePortions.size() == 5);
    SQLT_FUZZY_ASSERT(recipePortions[0], 2);
    SQLT_FUZZY_ASSERT(recipePortions[2], 4);
    SQLT_FUZZY_ASSERT(recipePortions[4], 4);

    std::vector<decltype(recipes_db::ingredients::name)> ingredientNames;
    result = SQLT::select<recipes_db, recipes_db::ingredients>(&recipes_db::ingredients::name, &ingredientNames, 5);
    SQLT_ASSERT(result == SQLITE_OK);
    SQLT_ASSERT(ingredientNames.size() == 10);
    SQLT_ASSERT(ingredientNames[3] == "Meatballs");
    SQLT_ASSERT(ingredientNames[7] == "Spices");
    SQLT_ASSERT(ingredientNames[9] == "Sausages");

    std::vector<decltype(recipes_db::recipes::favorite)> recipeFavorites;
    result = SQLT::select<recipes_db, recipes_db::recipes>(&recipes_db::recipes::favorite, &recipeFavorites, 5);
    SQLT_ASSERT(result == SQLITE_OK);
    SQLT_ASSERT(recipeFavorites.size() == 5);
    SQLT_ASSERT(recipeFavorites[0].is_null == false);
    SQLT_ASSERT(recipeFavorites[0].value == 0);
    SQLT_ASSERT(recipeFavorites[1].is_null == true);
    SQLT_ASSERT(recipeFavorites[4].is_null == false);
    SQLT_ASSERT(recipeFavorites[4].value == 1);

    std::vector<decltype(recipes_db::allergens::description)> allergenDescriptions;
    result = SQLT::select<recipes_db, recipes_db::allergens>(&recipes_db::allergens::description, &allergenDescriptions, 5);
    SQLT_ASSERT(result == SQLITE_OK);
    SQLT_ASSERT(allergenDescriptions.size() == 4);
    SQLT_ASSERT(allergenDescriptions[0].is_null == true);
    SQLT_ASSERT(allergenDescriptions[1].is_null == true);
    SQLT_ASSERT(allergenDescriptions[2].is_null == false);
    SQLT_ASSERT(allergenDescriptions[2].value == "All kinds of nuts.");
    SQLT_ASSERT(allergenDescriptions[3].is_null == false);
    SQLT_ASSERT(allergenDescriptions[3].value == "Deadly!");

    return 0;
}
