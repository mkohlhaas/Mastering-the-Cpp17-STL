#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>

class SeeAndSay
{
  private:
    // interface
    class AnimalConcept
    {
      public:
        virtual const char *see() const = 0;
        virtual const char *say() const = 0;
    };

    // model (implementation)
    template <typename T>
    class AnimalModel : public AnimalConcept
    {
      private:
        const T *m_animal; // hiding/erasure

      public:
        AnimalModel(const T *animal) : m_animal(animal)
        {
        }

        const char *
        see() const
        {
            return m_animal->see();
        }
        const char *
        say() const
        {
            return m_animal->say();
        }
    };

    std::vector<AnimalConcept *> m_animals;

  public:
    template <typename T>
    void
    addAnimal(T *animal)
    {
        m_animals.push_back(new AnimalModel(animal));
    }

    void
    pullTheString()
    {
        size_t index = rand() % m_animals.size();

        AnimalConcept *animal = m_animals[index];
        printf("The %s says '%s!'\n", animal->see(), animal->say());
    }
};

// some sample classes that implement the animal concept

struct Cow
{
    const char *
    see() const
    {
        return "cow";
    }

    const char *
    say() const
    {
        return "moo";
    }
};

struct Pig
{
    const char *
    see() const
    {
        return "pig";
    }

    const char *
    say() const
    {
        return "oink";
    }
};

struct Dog
{
    const char *
    see() const
    {
        return "dog";
    }

    const char *
    say() const
    {
        return "woof";
    }
};

int
main()
{
    srand(time(NULL));

    auto cow = Cow();
    auto pig = Pig();
    auto dog = Dog();

    auto sas = SeeAndSay();

    sas.addAnimal(&cow);
    sas.addAnimal(&pig);
    sas.addAnimal(&dog);

    sas.pullTheString();
}
