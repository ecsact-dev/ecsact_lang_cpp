# C++ Meta Header Code Generator

Generates a header containing information available at compile time about the Ecsact package. This includes all the components, transients, systems, actions and all the system/action capabilities. Use cases are not listed here, but this code generator is heavily used by the [EnTT Ecsact Runtime](https://github.com/ecsact-dev/ecsact_rt_entt).

## Design

Everything is built to work well with [boost.mp11](https://www.boost.org/doc/libs/master/libs/mp11/doc/html/mp11.html), but has no dependency with boost. [`ecsact::mp_list`](https://github.com/ecsact-dev/ecsact_runtime/blob/main/ecsact/lib.hh) is used as the base type to contain all our types.

The header contains a single struct named `package` inside the namespace that matches the Ecsact package name. The `package` struct contains the following types.

* `system_*_components` (replace `*` with system capability) which is a list of maps where the key is the system-like type and value is a list of component types matching the capability.

  ```ecsact
  package example;

  component Position { /* ... */ }
  component Health { /* ... */ }
  system MySystem {
    readwrite Position;
    readwrite Health;
  }
  ```

  ```cpp
  namespace example {
    struct package {
      // ...
      using system_readwrite_components = ecsact::mp_list<
        ecsact::mp_list<
          MySystem,
          ecsact::mp_list<
            Position,
            Health
          >
        >
      >;
    };
  }
  ```

* `system_association_*_components` (replace `*` with system capability) which is a list of maps where the key is the system-like type and the value is another map with the key being the associated component and the value being yet another map with the key being the associated field offset and the value being the list of component types matching the capability.

  ```ecsact
  package example;

  component Player;
  component Bleeding {
    i32 amount;
  }
  component Attacking {
    entity target_one;
    entity target_two;
  }
  action Attack {
    entity target;
    include Player;
    adds Attacking;
  }
  system AttackDamage {
    readonly Attacking {
      with target_one {
        readwrite Health;
        readwrite Bleeding;
      }
      with target_two {
        readwrite Health;
        readwrite Bleeding;
      }
    }
  }
  ```

  ```cpp
  namespace example {
    struct package {
      // ...
      using system_association_readwrite_components = ecsact::mp_list<
        ecsact::mp_list<
          AttackDamage,
          ecsact::mp_list<
            Attacking,
            ecsact::mp_list<
              ecsact::mp_list<
                offsetof(Attacking, target_one),
                ecsact::mp_list<
                  Health,
                  Bleeding
                >
              >,
              ecsact::mp_list<
                offsetof(Attacking, target_two),
                ecsact::mp_list<
                  Health,
                  Bleeding
                >
              >
            >
          >
        >
      >;
    };
  }
  ```
